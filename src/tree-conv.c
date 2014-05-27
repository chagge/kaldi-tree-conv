#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tree.h"
#include "question.h"
#include "file-utils.h"

#define DEBUG 0

#define TRUE 1
#define FALSE 0

struct str_slice {
    char ** elem;
    int size;
    int cap;
};

struct str_slice* str_slice_new()
{
    int init_cap = 500;
    struct str_slice *p = malloc(sizeof(struct str_slice));
    assert(p != NULL);
    p->elem = malloc(init_cap * sizeof(char *));
    p->size = 0;
    p->cap = init_cap;
    return p;
}

void str_slice_delete(struct str_slice *p)
{
    int i;
    for(i = 0; i != p->size; i++) {
        free(p->elem[i]);
    }
    free(p->elem);
    free(p);
}

void str_slice_append(struct str_slice *p, char *str)
{
    char **tmp;
    if (p->size == p->cap) {
        tmp = realloc(p->elem, p->cap * 2 * sizeof(p->elem[0]));
        assert(tmp != NULL);
        p->elem = tmp;
        p->cap *= 2;
    }
    p->elem[p->size++] = strdup(str);
}

int elem_in_array(int elem, int *array, int n) {
    int in = 0;
    int i = 0;
    for(i = 0; i != n; i++) {
        if (elem == array[i])
            in = 1;
    }
    return in;
}

int node_find_question_index(struct tree_node *np, struct question_set *qset)
{
    int i, j;

    int matched;
    for(i = 0; i < qset->size; i++) {
        matched = 1;
        struct question *q = &qset->elem[i];
        if (q->size != np->qsize) {
            matched = 0;
            continue;
        }
        for(j = 0; j < q->size; j++) {
            if ( ! elem_in_array(q->elem[j], np->question, np->qsize) ) {
                matched = 0;
            }
        }
        if (matched) {
            return i;
        }
    }
    return -1;
}

void read_phones(FILE *fp, struct str_slice *phones) 
{
    char line[512];
    char *phone, *index;
    while(fgets(line, sizeof(line), fp)) {
        //printf("%s", line);
        phone = strtok(line, " \n");
        if (phone[0] == '#') {
            continue;
        }
        str_slice_append(phones, phone);
        index = strtok(NULL, " \n");
        assert(phones->size == atoi(index) + 1);    // index is 0-based
    }
}

void question_set_write_htk(
        struct question_set *qset, 
        struct str_slice *phones, 
        FILE *fp) 
{ 
    int qi, ai;
    // L qset
    for(qi = 0; qi < qset->size; qi++) {
        struct question *q = &qset->elem[qi];

        fprintf(fp, "QS \'L_QUESTION_%d\' { ", qi);

        for(ai = 0; ai < q->size; ai++) {
            if (ai != 0) {
                fprintf(fp, ",");
            }
            fprintf(fp, "\"%s-*\"", phones->elem[q->elem[ai]]);
        }
        fprintf(fp, " }\n");
    }

    // R qset
    for(qi = 0; qi < qset->size; qi++) {
        struct question *q = &qset->elem[qi];

        fprintf(fp, "QS \'R_QUESTION_%d\' { ", qi);

        for(ai = 0; ai < q->size; ai++) {
            if (ai != 0) {
                fprintf(fp, ",");
            }
            fprintf(fp, "\"*+%s\"", phones->elem[q->elem[ai]]);
        }
        fprintf(fp, " }\n");
    }

    fprintf(fp, "\n#\n\n");
}

void node_mark_recursive(struct tree_node *np, int *counterp) 
{
    if (np == NULL) return;

    switch (np->type) {
    case CE:
        break;

    case SE:
        np->hook = (*counterp)++;
        assert(np->yes != NULL);
        assert(np->no != NULL);
        node_mark_recursive(np->no,  counterp);
        node_mark_recursive(np->yes, counterp);
        break;

    case TE:
        np->hook = (*counterp)++;
        int i;
        for (i = 0; i != np->nc; i++) {
            node_mark_recursive(np->children[i], counterp);
        }
        break;

    default:
        fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
        exit(0);
    }
}

void node_write_recursive_htk(
        struct tree_node *np, 
        struct question_set *qset, 
        FILE *fp)
{
    if (np == NULL) {
        fprintf(fp, "NULL ");
        return;
    }
    switch (np->type) {

    case CE:
        break;

    case SE:
        fprintf(fp, "%8d ", -np->hook);

        assert(np->yes != NULL); assert(np->no != NULL);

        int qi = node_find_question_index(np, qset);
        assert(qi != -1);

        // HTK decision trees only support LEFT & RIGHT questions
        // whereas Kaldi decision tree support LEFT, RIGHT, CENTRAL, STATE
        fprintf(fp, " \'%s_QUESTION_%d\' ", (np->key == LEFT) ? "L" : "R", qi);

        if (np->no->type == CE) {
            fprintf(fp, " \"PDFID_%d\"", np->no->pdf_id);
        } else {
            fprintf(fp, " -%d",    np->no->hook);
        }

        fprintf(fp, "    ");

        if (np->yes->type == CE) {
            fprintf(fp, " \"PDFID_%d\"", np->yes->pdf_id);
        } else {
            fprintf(fp, " -%d",    np->yes->hook);
        }
        fprintf(fp, "\n");

        node_write_recursive_htk(np->no,  qset, fp);
        node_write_recursive_htk(np->yes, qset, fp);
        break;

    case TE:
        fprintf(stderr, "%s line %d: ERROR: there is TE node when writing htk format tree, shouldn't happen\n", __func__, __LINE__);
        exit(0);

    default:
        fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
        exit(0);
    }
}

void tree_write_htk(
        struct tree *tp, 
        struct question_set *qset, 
        struct str_slice *phones, 
        FILE *fp)
{
    struct tree_node *root = tp->root;

    assert(root != NULL); assert(root->type == TE);

    // global tree first layer (phone TE map)
    int pi, si;
    for(pi = 0; pi != root->nc; pi++) {
        struct tree_node *p = root->children[pi];

        if (p == NULL) continue;  // this deals with "NULL" tree for <eps> in Kaldi

        assert(p->type == TE); assert(p->nc == 3);       

        // global tree second layer (state TE map)
        for(si = 0; si != p->nc; si++) {
            // subroot follows htk tree definition (phone, state)
            struct tree_node *subroot = p->children[si]; 

            fprintf(fp, "%s[%d]\n", phones->elem[pi], si + 2);   // si + 2 : HTK HMM initial state starts from index 2
            if (subroot->type == CE) {
                fprintf(fp, "    \"PDFID_%d\"\n", subroot->pdf_id);

            } else {
                int counter = 0;
                node_mark_recursive(subroot, &counter);

                fprintf(fp, "{\n");   
                node_write_recursive_htk(subroot, qset, fp);
                fprintf(fp, "}\n");   
            }
            fprintf(fp, "\n");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "./tree-conv (i)KALDI_tree.txt (i)phones.txt (i)questions.int (o)HTK_tree.txt\n");
        return EXIT_SUCCESS;
    }

    char *itree_filename     = argv[1];
    char *iphones_filename   = argv[2];
    char *iqustions_filename = argv[3];
    char *otree_filename     = argv[4];

    FILE *fp;

    // read kaldi phone list
    fp = fopen(iphones_filename, "r");      assert(fp != NULL);
    struct str_slice *phones = str_slice_new();
    read_phones(fp, phones);
    fclose(fp);

    if (DEBUG) {
        int i;
        fprintf(stderr, "<PHONES>: %d\n", phones->size);
        for(i = 0; i != phones->size; i++) {
            fprintf(stderr, "%s\n", phones->elem[i]);
        }
        fprintf(stderr, "</PHONES>\n\n");
    }

    // read kaldi qset
    fp = fopen(iqustions_filename, "r");    assert(fp != NULL);
    struct question_set *qset = question_set_new();
    question_set_load(qset, fp);
    fclose(fp);

    if (DEBUG) {
        fprintf(stderr, "<questions>: %d\n", qset->size);
        question_set_display(qset);
        fprintf(stderr, "</questions>\n\n");
    }

    // read kaldi tree
    fp = fopen(itree_filename, "r");
    assert(fp != NULL);
    struct tree *tp = tree_new();
    tree_load(tp, fp);
    fclose(fp);

    //// write out as Kaldi format
    //TreeWriteKaldi(tp, ofp);

    // write tree as htk format
    FILE *ofp = fopen(otree_filename, "w+");
    question_set_write_htk(qset, phones, ofp);
    tree_write_htk(tp, qset, phones, ofp);
    fclose(ofp);

    str_slice_delete(phones);
    question_set_delete(qset);
    tree_delete(tp);

    return EXIT_SUCCESS;
}


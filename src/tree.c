#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "file-utils.h"
#include "tree.h"

struct tree_node* tree_node_load_recursive(FILE *fp, char *tok);    // forward declaration

struct tree_node* tree_node_new(NODE_TYPE type) 
{
    struct tree_node *np = malloc(sizeof(struct tree_node));
    assert(np != NULL);
    switch (type) {
        case CE:
            np->type = CE;
            np->pdf_id = -1;    /* KALDI pdf_id is 0-based */
            break;

        case SE:
            np->type = SE;

            np->question = NULL;
            np->qsize = 0;

            np->yes = NULL;
            np->no  = NULL;
            break;

        case TE:
            np->type = TE;

            np->children = NULL;
            np->nc = 0;
            break;

        default:
            fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
            exit(0);
    }
    return np;
}

void tree_node_delete(struct tree_node *np) 
{
    if (np == NULL) return;

    switch (np->type) {
        case CE:
            free(np); break;
        case SE: 
            free(np->question);
            free(np); break;
        case TE:
            free(np->children);
            free(np); break;
        default:
            fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
            exit(0);
    }
    return;
}

void tree_node_delete_recursive(struct tree_node *np)
{
    if (np == NULL) return;
    int i;
    switch (np->type) {
        case CE: 
            tree_node_delete(np);
            break;
        case SE: 
            tree_node_delete_recursive(np->yes);
            tree_node_delete_recursive(np->no);
            tree_node_delete(np);
            break;
        case TE: 
            for(i = 0; i != np->nc; i++) {
                tree_node_delete_recursive(np->children[i]);
            }
            tree_node_delete(np);
            break;
        default: 
            fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
            exit(0);
    }
}

// KALDI quasi-BNF grammar for CE node:  
// "CE" <numeric pdf-id>
void tree_node_load_ce(struct tree_node *np, FILE *fp, char *tok)
{
    expect_token(fp, " \n", "CE");
    read_token(fp, " \n", tok);  np->pdf_id = atoi(tok);
}

// KALDI quasi-BNF grammar for SE node:
// "SE" <key-to-split-on> "[" yes-value-list "]" "{" EventMap EventMap "}"
void tree_node_load_se(struct tree_node *np, FILE *fp, char *tok)
{
    expect_token(fp, " \n", "SE");
    read_token(fp, " \n", tok);  np->key = atoi(tok);
    expect_token(fp, " \n", "[");

    // count question list number
    int qsize = 0;
    fpos_t pos;
    fgetpos(fp, &pos);
    for (read_token(fp, " \n", tok); strcmp(tok, "]") != 0; read_token(fp, " \n", tok)) {
        qsize++;
    }

    // allocate for question list
    np->qsize = qsize;
    np->question = malloc(qsize * sizeof(int));
    assert(np->question != NULL);

    // read question list
    fsetpos(fp, &pos);
    int i;
    for(i = 0; i != np->qsize; i++) {
        read_token(fp, " \n", tok);
        np->question[i] = atoi(tok);
    }

    expect_token(fp, " \n", "]");
    expect_token(fp, " \n", "{");
    np->yes = tree_node_load_recursive(fp, tok);
    np->no  = tree_node_load_recursive(fp, tok);
    expect_token(fp, " \n", "}");
}

// KALDI quasi-BNF grammar for TE node:  
// "TE" <key-to-split-on> <table-size> "(" EventMapList ")"
void tree_node_load_te(struct tree_node *np, FILE *fp, char *tok)
{
    expect_token(fp, " \n", "TE");
    read_token(fp, " \n", tok); np->key = atoi(tok); 
    read_token(fp, " \n", tok); np->nc  = atoi(tok); 

    np->children = malloc(np->nc * sizeof(struct tree_node *));
    assert(np->children != NULL);

    expect_token(fp, " \n", "(");
    int i;
    for(i = 0; i != np->nc; i++) {
        np->children[i] = tree_node_load_recursive(fp, tok);
    }
    expect_token(fp, " \n", ")");
}

struct tree_node* tree_node_load_recursive(FILE *fp, char *tok)
{
    struct tree_node* np = NULL;
    peek_token(fp, " \n", tok);
    if (strcmp(tok, "CE") == 0) {
        np = tree_node_new(CE);
        tree_node_load_ce(np, fp, tok);

    } else if (strcmp(tok, "SE") == 0) {
        np = tree_node_new(SE);
        tree_node_load_se(np, fp, tok);

    } else if (strcmp(tok, "TE") == 0) {
        np = tree_node_new(TE);
        tree_node_load_te(np, fp, tok);

    } else if (strcmp(tok, "NULL") == 0) {
        expect_token(fp, " \n", "NULL");

    } else {
        fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
        exit(0);
    }
    return np;
}

struct tree* tree_new()
{
    struct tree *tp = malloc(sizeof(struct tree));
    assert(tp != NULL);
    tp->N = 0; tp->P = 0;   // for typical triphone N=3, P=1
    tp->root = NULL;
    return tp;
}

void tree_delete(struct tree *tp) 
{
    tree_node_delete_recursive(tp->root);
    free(tp);
}

void tree_load(struct tree *tp, FILE* fp)
{
    char tok[256];
    expect_token(fp, " \n", "ContextDependency");

    read_token(fp, " \n", tok);    tp->N = atoi(tok);
    read_token(fp, " \n", tok);    tp->P = atoi(tok);
    fprintf(stderr, "context info: N=%d P=%d\n", tp->N, tp->P);

    expect_token(fp, " \n", "ToPdf");
    tp->root = tree_node_load_recursive(fp, tok);
    expect_token(fp, " \n", "EndContextDependency");
}

void tree_node_write_recursive(struct tree_node *np, FILE *fp)
{
    if (np == NULL) {
        fprintf(fp, "NULL ");
        return;
    }
    int i = 0;

    switch (np->type) {
    case CE:
        fprintf(fp, "CE %d ", np->pdf_id); 
        break;

    case SE:
        fprintf(fp, "SE %d [ ", np->key);
        for(i = 0; i != np->qsize; i++) {
            fprintf(fp, "%d ", np->question[i]);
        }
        fprintf(fp, "]\n");
        fprintf(fp, "{ ");

        assert(np->yes != NULL);
        assert(np->no  != NULL);
        tree_node_write_recursive(np->yes, fp);
        tree_node_write_recursive(np->no, fp);

        fprintf(fp, "} \n");
        break;

    case TE:
        fprintf(fp, "TE %d %d ( ", np->key, (int)np->nc);
        for (i = 0; i != np->nc; i++) {
            tree_node_write_recursive(np->children[i], fp);
        }
        fprintf(fp, ") \n");
        break;

    default:
        fprintf(stderr, "%s line %d: unknown node type.\n", __func__, __LINE__);
        exit(0);
    }
}

void tree_write_kaldi(struct tree *tp, FILE *fp)
{
    fprintf(fp, "ContextDependency %d %d ToPdf ", tp->N, tp->P);
    tree_node_write_recursive(tp->root, fp);
    fprintf(fp, "EndContextDependency ");
}


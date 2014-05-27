#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "question.h"

struct question_set* question_set_new()
{
    struct question_set* qset = malloc(sizeof(struct question_set));
    assert(qset != NULL);
    qset->elem = NULL;
    qset->size  = 0;
    qset->cap   = 0;
    return qset;
}

void question_set_delete(struct question_set* qset)
{
    int i;
    for(i = 0; i < qset->size; i++) {
        struct question *q = &qset->elem[i];
        free(q->elem); q->elem = NULL;
    }
    free(qset->elem); qset->elem = NULL;
    free(qset);
}

void question_set_load(struct question_set *qset, FILE *fp) 
{
    char line[2048];
    char *token;
    while(fgets(line, sizeof(line), fp)) {

        if (qset->size == qset->cap) {
            int new_cap = qset->cap * 2 + 10;
            void *tmp = realloc(qset->elem, new_cap * sizeof(struct question));
            assert(tmp != NULL);
            qset->elem = tmp;
            qset->cap = new_cap;
        }

        struct question *q = &qset->elem[qset->size++];
        q->elem = NULL;
        q->size  = 0;
        q->cap   = 0;

        int ans;
        for(token = strtok(line, " "); token != NULL; token = strtok(NULL, " ")) {
            ans = atoi(token);
            if (q->size == q->cap) {
                int new_cap = q->cap * 2 + 10;
                void *tmp = realloc(q->elem, new_cap * sizeof(int));
                assert(tmp != NULL);
                q->elem = tmp;
                q->cap = new_cap;
            }
            q->elem[q->size] = ans;
            q->size++;
        }
    }
}

void question_set_display(struct question_set *qset)
{
    int qi, ai;
    for(qi = 0; qi < qset->size; qi++) {
        printf("%d:", qi);
        struct question *q = &qset->elem[qi];
        for(ai = 0; ai < q->size; ai++) {
            if (ai != 0) printf(" ");
            printf("%d", q->elem[ai]);
        }
        printf("\n");
    }
}

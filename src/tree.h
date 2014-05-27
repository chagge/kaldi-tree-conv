#ifndef TREE_H
#define TREE_H

typedef enum {TE, SE, CE} NODE_TYPE;
typedef enum { STATE=-1, LEFT=0, CENTRAL=1, RIGHT=2 } KEY;

struct tree_node {
    NODE_TYPE type;
    KEY  key; 
    int hook;

    /* TE - Table Event Map Node */
    struct tree_node **children;
    size_t nc;

    /* SE - Split Event Map Node */
    int *question;
    size_t qsize;

    struct tree_node *yes;
    struct tree_node *no;

    /* CE - Constant Event Node (tree-leaf) */
    int pdf_id;
};

struct tree {
    int N, P;
    struct tree_node *root;
};

struct tree* tree_new();
void tree_load(struct tree *tp, FILE* fp);
void tree_write_kaldi(struct tree *tp, FILE *fp);
void tree_delete(struct tree *tp);

#endif

#ifndef QUESTION_H
#define QUESTION_H

struct question {
    int *elem;
    int size;
    int cap;
};

struct question_set {
    struct question *elem;
    int size;
    int cap;
};

struct question_set* question_set_new();
void question_set_load(struct question_set *qset, FILE *fp);
void question_set_display(struct question_set *qset);
void question_set_delete(struct question_set* qset);

#endif

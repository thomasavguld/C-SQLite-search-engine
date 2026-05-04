#ifndef DOCUMENT_EVENT_H
#define DOCUMENT_EVENT_H

#define MAX_AUTHORS 32

typedef struct {
    const char *first;
    const char *last;
    const char *initial;
} AuthorEvent;

typedef struct {
    const char *doi;
    const char *title;
    const char *abstract;
    const char *issn;
    int pub_year;

    AuthorEvent authors[MAX_AUTHORS];
    int author_count;

} DocumentEvent;

#endif
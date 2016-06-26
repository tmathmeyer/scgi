#ifndef status_h
#define status_h

typedef enum {
    html,
    text,
} content_type_t;

void scgi_content(content_type_t content);
void scgi_status(int status);
void scgi_begin_body(void);

#endif

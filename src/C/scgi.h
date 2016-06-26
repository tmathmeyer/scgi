#ifndef _scgi_h_
#define _scgi_h_

#include <stdint.h>

#define err(msg) \
    do { \
        printf("%s:%i >> %s\n", __FILE__, __LINE__, (msg)); \
        exit(1); \
    } while(0)

typedef struct {
    uint8_t is_default;
    size_t strlen;
    char *name;
    char *binary;
} entry_t;

typedef struct {
    size_t numentries;
    entry_t *entries;
} config_t;

void exec_redir(char *, int);
config_t *parse_config(char *);
void pflags(int, char **, char ***);

#endif

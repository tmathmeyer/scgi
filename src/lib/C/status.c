#include <stdio.h>
#include "../H/status.h"

void scgi_status(int status) {
    char *ststr = NULL;
    switch(status) {
        case 200:
            ststr = "OK"; break;
        default:
            ststr = "UNKNOWN"; break;
    }
    printf("HTTP/1.1 %i %s\r\n", status, ststr);
    printf("Server: scgi/1.0\r\n");
}


void scgi_content(content_type_t content) {
    printf("Content-Type: ");
    switch(content) {
        case html:
            printf("text/html"); break;
        case text:
            printf("text/plain"); break;
    }
    printf("\r\n");
}

void scgi_begin_body(void) {
    printf("\r\n");
}

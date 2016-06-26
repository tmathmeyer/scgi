#define CONFIG_FLAG 0
#define SOCKET_FLAG 1
#define FLAGS_N 2

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/sockios.h>
#include <errno.h>
#include <signal.h>
#include "scgi.h"

static int sockfd;
static int running = 1;

void intHandler(int dummy) {
    close(sockfd);
    puts("killing process!");
    running = 0;
}

#define BUFSIZE 64
typedef struct {
    int socket;
    char buffer[BUFSIZE];
    size_t buffer_terminal;
    size_t buffer_read_index;
} socket_buffer_t;

void ins_char(char **res, size_t *index, size_t *guess, char ins) {
    if (*index == *guess) {
        *guess *= 1.618; //golden ratiooooo
        *res = realloc(*res, *guess);
        memset(*res+*index, 0, *guess-*index);
    }
    (*res)[*index] = ins;
    *index = *index + 1;
    printf("%c", ins);
}

char *read_until(char *str, socket_buffer_t *buf, char include_match) {
    size_t size_guess = 8;
    size_t result_index = 0;
    char *result;
    char *str_orig = str;
    if (str != 0) {
        result = calloc(size_guess, sizeof(char));
    }

    for(;;) {
        if (buf->buffer_read_index >= buf->buffer_terminal) {
            if (str == 0) {
                return NULL;
            }
            buf->buffer_read_index = 0;
            buf->buffer_terminal = read(buf->socket, buf->buffer, BUFSIZE);
        }
        size_t b = buf->buffer_read_index;

        if (buf->buffer_terminal == 0) {
            if (result_index == 0) {
                if (str != 0) {
                    free(result);
                }
                return NULL;
            }
            return result;
        } 
        if (str != 0) {
            if (*str == 0) {
                buf->buffer_read_index = b;
                if (!include_match) {
                    result[strlen(result)-strlen(str_orig)] = 0;
                }
                return result;
            } else {
                ins_char(&result, &result_index, &size_guess, buf->buffer[b]);
                if (buf->buffer[b] == *str) {
                    str++;
                } else {
                    str = str_orig;
                }
            }
        } else {
            printf("%c", buf->buffer[b]);
        }
        buf->buffer_read_index = b+1;
    }
}

char *read_including(char *str, socket_buffer_t *buf) {
    return read_until(str, buf, 0);
}

socket_buffer_t make_buffer(int socket) {
    socket_buffer_t t;
    memset(&t, 0, sizeof(socket_buffer_t));
    t.socket = socket;
    return t;
}

char *get_binary_for(char *host_val, config_t *co) {
    size_t hostlen = strlen(host_val);
    for(ssize_t i=0;i<co->numentries;i++) {
        if (!(co->entries)[i].is_default) {
            if (co->entries[i].strlen == hostlen) {
                if (!strcmp(co->entries[i].name, host_val)) {
                    return co->entries[i].binary;
                }
            }
        }
    }
    return NULL;
}

void open_socket(char *c, config_t *co) {
    char *def_bin = 0;
    int newsockfd, portno, n;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    for(ssize_t i=0;i<co->numentries;i++) {
        if ((co->entries)[i].is_default) {
            def_bin = (co->entries)[i].binary;
            printf("default binary is: %s\n", def_bin);
        }
    }

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err("cant open socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(c);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
        err("cant bind socket");
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while(running) {
        puts("================================");
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("cant accept socket");
            close(sockfd);
            return;
        }
        socket_buffer_t sock = make_buffer(newsockfd);
        char *method = read_until(" ", &sock, 0);
        char *path = read_until(" ", &sock, 0);
        char *httpver = read_until("\r\n", &sock, 0);

        char *_host = read_until("Host: ", &sock, 1);
        char *host = read_until("\r\n", &sock, 0);
        
        if (read_until(0, &sock, 0)) {
            puts("UH OH");
        }

        do {
            bzero(buffer, 256);
            n = read(newsockfd, buffer, 255);
            if (n < 0) {
                err("cant read from socket");
            }
            printf("%s\n", buffer);
        } while(n == 255);

        //host_val[strlen(host_val)-1] = 0;
        char *bin = get_binary_for(host, co);
        printf("binary to exec: %s\n", bin);

        printf("method      = %s\n", method);
        printf("path        = %s\n", path);
        printf("httpver     = %s\n", httpver);
        printf("_host       = %s\n", _host);
        printf("host        = %s\n", host);
        
        free(method);
        free(path);
        free(httpver);
        free(_host);
        free(host);

        if (bin) {
            exec_redir(bin, newsockfd);
        } else if (def_bin) {
            exec_redir(def_bin, newsockfd);
        }
    }
    close(sockfd);
}

void exec_redir(char *binary, int socket) {
    printf("executing: (%s)\n", binary);
    int link[2];
    pid_t pid;
    char buf[4096];

    if (pipe(link)==-1) {
        err("pipe");
    }

    if ((pid = fork()) == -1) {
        err("fork");
    }

    if(pid == 0) {
        dup2 (link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execl(binary, binary, NULL);
    } else {
        close(link[1]);
        int bytes = 0;
        do {
            bytes = read(link[0], buf, sizeof(buf));
            write(socket, buf, bytes);
        } while(bytes);
        int pending = 0;
        while(ioctl(socket, SIOCOUTQ, &pending),pending);
        close(socket);
        wait(NULL);
        //waitpid(pid, NULL, 0);
    }
}


int main(int argc, char **argv) {
    signal(SIGINT, intHandler);
    char **flags = 0;
    pflags(argc, argv, &flags);

    if (!flags[CONFIG_FLAG] || !flags[SOCKET_FLAG]) {
        err("missing flags");
    }

    config_t *config = parse_config(flags[CONFIG_FLAG]);
    open_socket(flags[SOCKET_FLAG], config);
}

#define flag_bind(f, l) \
    if (!strcmp(argv[i], (f))) (*flags)[(l)] = argv[++i];

void pflags(int argc, char **argv, char ***flags) {
    *flags = calloc(FLAGS_N, sizeof(char **));
    if (!*flags) {
        err("allocation error");
    }
    ssize_t i = 0;
    while(i<argc) {
        flag_bind("-c", CONFIG_FLAG);
        flag_bind("-p", SOCKET_FLAG);
        i++;
    }
}

int alphanumdot(char c) {
    if (c == '.') return 1;
    if (c == ':') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    return 0;
}

config_t *parse_config(char *location) {
    FILE *f = fopen(location, "r");
    if (f == NULL) {
        err("bad_file");
    }

    config_t *result = calloc(1, sizeof(config_t));
    if (!result) {
        err("allocation_error");
    }
    ssize_t eb_size  = 4;
    ssize_t eb_count = 0;
    entry_t *eb_mem = calloc(eb_size, sizeof(entry_t));
    if (!eb_mem) {
        err("allocation_error");
    }

    char strbuffer[100] = {0};
    ssize_t sb_loc = 0;
    char c = EOF;
    ssize_t RUNNING = 1;

#define M_HOST 0
#define M_WHTS 1
#define M_BIN  2
    ssize_t MODE = M_HOST;
    while(RUNNING&&(c=fgetc(f))) {
        if (MODE == M_HOST) {
            if (c == EOF) {
                RUNNING=0;
            } else if (alphanumdot(c)) {
                strbuffer[sb_loc++] = c;
            } else {
                if (sb_loc == 0) {
                    err("parse_error");
                } else if (eb_count==eb_size) {
                    eb_size *=2;
                    eb_mem = realloc(eb_mem, eb_size*sizeof(entry_t));
                    if (!eb_mem) {
                        err("allocation_error");
                    }
                    memset(eb_mem+eb_count, 0, eb_count*sizeof(entry_t));
                } else if (!strcmp(strbuffer, "DEFAULT")) {
                    eb_mem[eb_count].is_default = 1;
                } else {
                    eb_mem[eb_count].strlen = sb_loc;
                    eb_mem[eb_count].name = strndup(strbuffer, sb_loc);
                }
                sb_loc = 0;
                memset(strbuffer, 0, 100);
                MODE = M_WHTS;
            }
        }
        if (MODE == M_WHTS) {
            if (!(c==' ' || c=='\t')) {
                MODE = M_BIN;
                goto binary_path_parse_start;
            } else if (c == EOF) {
                err("eof_error");
            }
        }
        if (MODE == M_BIN) {
binary_path_parse_start:
            if (c == EOF) {
                eb_mem[eb_count].binary = strndup(strbuffer, sb_loc);
                MODE=M_HOST;
                eb_count++;
                RUNNING=0;
            } else if (c == '\n') {
                eb_mem[eb_count].binary = strndup(strbuffer, sb_loc);
                MODE=M_HOST;
                eb_count++;
                sb_loc = 0;
                memset(strbuffer, 0, 100);
            } else {
                strbuffer[sb_loc++] = c;
            }
        }
    }
#undef M_HOST
#undef M_WHTS
#undef M_BIN
    fclose(f);
    result->numentries = eb_count;
    result->entries = eb_mem;
    return result;
}

#include <scgi/status.h> // run make install in the project to build with this
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int dir_in_order(char *dir);
int main(int argc, char **argv) {
    scgi_status(200);
    scgi_content(html);
    scgi_begin_body();

    printf("<html><body>\n");
    for(int i=1; i<argc; i++) {    
        dir_in_order(argv[i]);
    }
    printf("</body></html>");
}

int sbd(const struct dirent **a, const struct dirent **b) {
    struct stat b_stat;
    struct stat a_stat;
    if (stat((*a)->d_name, &a_stat) == -1) {
        perror((*a)->d_name);
        exit(1);
    }
    if (stat((*b)->d_name, &b_stat) == -1) {
        perror((*b)->d_name);
        exit(1);
    }

    return b_stat.st_mtime - a_stat.st_mtime;
}

int dir_in_order(char *dir) {
    char *old = getcwd(NULL, 0);
    chdir(dir);
    struct dirent **entry_list;
    int count;
    int i;

    count = scandir(dir, &entry_list, 0, sbd);
    if (count < 0) {
        perror("scandir");
        return 1;
    }

    for (i = 0; i < count; i++) {
        struct dirent *entry;
        entry = entry_list[i];
        printf("<a href='http://files.tmathmeyer.me/%s'>", entry->d_name);
        printf("%s</a><br/>\n", entry->d_name);
        free(entry);
    }
    free(entry_list);
    chdir(old);
    free(old);

    return 0;
}

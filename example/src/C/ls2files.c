#include <scgi/status.h> // run make install in the project to build with this
#include <stdio.h>
#include <dirent.h>

int print_dirs(char *path) {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (*(dir->d_name) != '.') {
                printf("<a href='http://files.tmathmeyer.me/%s'>", dir->d_name);
                printf("%s</a><br/>\n", dir->d_name);
            }
        }
        closedir(d);
    }
    return(0);
}

int main(int argc, char **argv) {
    scgi_status(200);
    scgi_content(html);
    scgi_begin_body();

    printf("<html><body>\n");
    print_dirs(".");
    printf("</body></html>");
}

#include "files.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */

static int has_png_ext(const char *name) {
    size_t n = strlen(name);
    if (n < 4) return 0;
    const char *ext = name + n - 4;
    return strcasecmp(ext, ".png") == 0;
}

char **files_list_png(size_t *count) {
    *count = 0;
    DIR *d = opendir(".");
    if (!d) {
        perror("opendir");
        return NULL;
    }

    size_t cap = 16, n = 0;
    char **list = malloc(cap * sizeof(char *));
    if (!list) { closedir(d); return NULL; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!has_png_ext(ent->d_name)) continue;

        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(list, cap * sizeof(char *));
            if (!tmp) {
                files_free_list(list, n);
                closedir(d);
                return NULL;
            }
            list = tmp;
        }
        list[n] = malloc(strlen(ent->d_name) + 1);
        if (!list[n]) {
            files_free_list(list, n);
            closedir(d);
            return NULL;
        }
        strcpy(list[n], ent->d_name);
        n++;
    }
    closedir(d);

    *count = n;
    return list;
}

void files_free_list(char **list, size_t count) {
    if (!list) return;
    for (size_t i = 0; i < count; ++i) free(list[i]);
    free(list);
}

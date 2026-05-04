#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "fs.h"

#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, size, f);
    buf[size] = '\0';

    fclose(f);
    return buf;
}

/* -------------------------------------------------- */
/* internal recursive walker                          */
/* -------------------------------------------------- */

static void walk(const char *path, file_callback cb, void *userdata)
{
    struct stat st;

    if (stat(path, &st) != 0)
        return;

    /* ---- file ---- */
    if (S_ISREG(st.st_mode)) {
        cb(path, userdata);
        return;
    }

    /* ---- directory ---- */
    if (!S_ISDIR(st.st_mode))
        return;

    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        /* skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        /* build full path */
        size_t len = strlen(path) + strlen(entry->d_name) + 2;
        char *fullpath = malloc(len);

        if (!fullpath)
            continue;

        snprintf(fullpath, len, "%s/%s", path, entry->d_name);

        walk(fullpath, cb, userdata);

        free(fullpath);
    }

    closedir(dir);
}

/* -------------------------------------------------- */
/* public API                                         */
/* -------------------------------------------------- */

void list_files(const char *root, file_callback cb, void *userdata)
{
    if (!root || !cb)
        return;

    walk(root, cb, userdata);

}
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cpio_t {
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
};

/**
 * @brief Convert a hexadecimal string to integer
 *
 * @param s hexadecimal string
 * @param n length of the string
 * @return integer value
 */
static int hextoi(const char* s, int n) {
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= 0)
            r += *s++ - '0';
    }
    return r;
}

/**
 * @brief Align a number to the nearest multiple of a given number
 *
 * @param n number
 * @param byte alignment
 * @return aligned number
 */
static int align(int n, int byte) {
    return (n + byte - 1) & ~(byte - 1);
}

void initrd_list(const void* rd) {
    int off = 0;                  // offset of the current record in the archive

    while (1) {
        const struct cpio_t* h = (const struct cpio_t*)((const char*)rd + off);

        // magic is 6 chars and NOT '\0'-terminated, so compare a fixed length
        if (strncmp(h->magic, "070701", 6) != 0) {
            fprintf(stderr, "initrd_list: bad magic at offset %d\n", off);
            return;
        }

        int filesize = hextoi(h->filesize, 8);
        int namesize = hextoi(h->namesize, 8);

        // the file name sits right after the header; h + 1 steps over
        // sizeof(struct cpio_t) bytes because of pointer arithmetic
        const char* name = (const char*)(h + 1);

        // the archive ends with a record named "TRAILER!!!"
        if (strcmp(name, "TRAILER!!!") == 0) break;

        printf("%6d %s\n", filesize, name);

        // advance: header + name + padding, then data + padding
        off += align(sizeof(struct cpio_t) + namesize, 4);
        off += align(filesize, 4);
    }
}

void initrd_cat(const void* rd, const char* filename) {
    const char* base = (const char*)rd;
    int off = 0;

    while (1) {
        const struct cpio_t* h = (const struct cpio_t*)(base + off);

        if (strncmp(h->magic, "070701", 6) != 0) {
            fprintf(stderr, "initrd_cat: bad magic at offset %d\n", off);
            return;
        }

        int filesize = hextoi(h->filesize, 8);
        int namesize = hextoi(h->namesize, 8);
        const char* name = (const char*)(h + 1);

        int name_pad = align(sizeof(struct cpio_t) + namesize, 4);

        if (strcmp(name, "TRAILER!!!") == 0) {
            // reached the end of the archive without a match
            fprintf(stderr, "initrd_cat: %s: No such file\n", filename);
            return;

        } else if (strcmp(name, filename) == 0) {
            // the data sits right after the header, the name and its padding.
            // it is not '\0'-terminated, so print exactly filesize bytes.
            const char* data = base + off + name_pad;
            printf("%.*s", filesize, data);
            return;

        } else {
            // not this one, move on to the next record
            off += name_pad;
            off += align(filesize, 4);
        }
    }
}
int main() {
    /* Prepare the initial RAM disk */
    FILE* fp = fopen("initramfs.cpio", "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    void* rd = malloc(sz);
    fseek(fp, 0, SEEK_SET);
    if (fread(rd, 1, sz, fp) != sz) {
        fprintf(stderr, "Failed to read the device tree blob\n");
        free(rd);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);

    initrd_list(rd);
    initrd_cat(rd, "osc.txt");
    initrd_cat(rd, "test.txt");

    free(rd);
    return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

static inline uint32_t bswap32(uint32_t x) {
    return __builtin_bswap32(x);
}

static inline uint64_t bswap64(uint64_t x) {
    return __builtin_bswap64(x);
}

static inline const void* align_up(const void* ptr, size_t align) {
    return (const void*)(((uintptr_t)ptr + align - 1) & ~(align - 1));
}

static inline uint32_t read_be32(const void* p) {
    return bswap32(*(const uint32_t*)p);
}

#define MAX_DEPTH 16
#define FDT_MAGIC 0xd00dfeed

// node_name: node name from the blob ('\0'-terminated)
// comp / comp_len: one component of the path (NOT '\0'-terminated)
static int name_match(const char* node_name, const char* comp, int comp_len) {
    int cmp_len;   // how much of node_name should take part in the comparison

    if (memchr(comp, '@', comp_len)) {
        // comp has a unit address -> exact match on the whole name
        cmp_len = strlen(node_name);
    } else {
        // comp has no unit address -> match only the part before '@'
        const char* at = strchr(node_name, '@');
        cmp_len = at ? (int)(at - node_name) : (int)strlen(node_name);
    }

    return cmp_len == comp_len && strncmp(node_name, comp, comp_len) == 0;
}

int fdt_path_offset(const void* fdt, const char* path) {
    const struct fdt_header* h = fdt;
    if (bswap32(h->magic) != FDT_MAGIC) return -1;

    /* --- Split the path into components --- */
    // The leading '/' yields an empty first component, which matches the
    // root node (whose name in the blob is the empty string).
    const char* comp[MAX_DEPTH];   // start of each component
    int comp_len[MAX_DEPTH];       // length of each component
    int n = 0;                     // number of components
    const char* s = path;
    while (1) {
        if (n >= MAX_DEPTH) return -1;
        const char* slash = strchr(s, '/');
        comp[n] = s;

        if (slash) {
            // more components follow
            comp_len[n] = slash - s ;
            n++;
            s = slash + 1;


        } else {
            // last component
            comp_len[n] = strlen(s);
            n++;
            break;
        }
    }

    /* --- Walk the structure block, matching the path as we go --- */
    const char* struct_base = (const char*)fdt + bswap32(h->off_dt_struct);

    const char* p = struct_base;
    int depth = 0;      // depth of the node we are looking at (root = 0)
    int matched = 0;    // how many path components matched so far

    while (1) {
        const char* tok_start = p;
        uint32_t token = read_be32(p);
        p += 4;

        if (token == FDT_BEGIN_NODE) {
            const char* name = p;

            if (depth == matched && matched < n &&
                name_match(name, comp[matched], comp_len[matched])) {
                matched++;
                if (matched == n) return (int)(tok_start - struct_base);
            }

            p += strlen(name) + 1;
            p = (const char*)align_up(p, 4);
            depth++;

        } else if (token == FDT_END_NODE) {
            depth--;
            if (matched > depth) matched = depth;   // left the branch, roll progress back

        } else if (token == FDT_PROP) {
            uint32_t len = read_be32(p);            // nameoff is not needed here
            p += 8;
            p += len;
            p = (const char*)align_up(p, 4);

        } else if (token == FDT_NOP) {
            // ignored

        } else if (token == FDT_END) {
            return -1;                              // reached the end without a match

        } else {
            return -1;                              // unknown token
        }
    }
}

const void* fdt_getprop(const void* fdt, int nodeoffset,
                        const char* name, int* lenp) {
    const struct fdt_header* h = fdt;
    if (bswap32(h->magic) != FDT_MAGIC) return NULL;
    if (nodeoffset < 0) return NULL;

    const char* struct_base  = (const char*)fdt + bswap32(h->off_dt_struct);
    const char* strings_base = (const char*)fdt + bswap32(h->off_dt_strings);

    // nodeoffset points at the node's FDT_BEGIN_NODE token;
    // skip the token and the node name to reach the first property
    const char* p = struct_base + nodeoffset;
    p += 4;
    p += strlen(p) + 1;
    p = (const char*)align_up(p, 4);

    while (1) {
        uint32_t token = read_be32(p);
        p += 4;

        if (token == FDT_PROP) {
            // layout: len (4B) + nameoff (4B) + data (len bytes) + padding
            uint32_t len     = read_be32(p);
            uint32_t nameoff = read_be32(p + 4);
            p += 8;                                     // p now points at the data
            const char* prop_name = strings_base + nameoff;

            if (strcmp(prop_name, name) == 0) {
                if (lenp) *lenp = (int)len;
                return p;
            }

            p += len;                                   // not this one, skip it
            p = (const char*)align_up(p, 4);

        } else if (token == FDT_NOP) {
            continue;

        } else {
            // properties always come before child nodes, so BEGIN_NODE /
            // END_NODE / END all mean this node has no more properties
            return NULL;
        }
    }
}
void dump_header(const void* fdt) {
    // print magic, off_dt_struct and off_dt_strings
    const struct fdt_header* h = fdt;
    uint32_t magic = bswap32(h->magic);
    uint32_t off_dt_struct = bswap32(h->off_dt_struct);
    uint32_t off_dt_strings = bswap32(h->off_dt_strings);
    
    printf("magic = 0x%x\n", magic);
    printf("off_dt_struct = 0x%x\n", off_dt_struct);
    printf("off_dt_strings = 0x%x\n", off_dt_strings);
}

void dump_tree(const void* fdt) {
    const struct fdt_header* h = fdt;
    const char* struct_base  = (const char*)fdt + bswap32(h->off_dt_struct);
    const char* strings_base = (const char*)fdt + bswap32(h->off_dt_strings);

    const char* p = struct_base;
    int depth = 0;

    while (1) {
        const char* tok_start = p;          // remember where the token starts
        uint32_t token = read_be32(p);
        p += 4;                             // skip the token itself

        if (token == FDT_BEGIN_NODE) {
            const char* name = p;
            long offset = tok_start - struct_base;
            printf("[%4ld] %*s%s\n", offset, depth * 2, "", name);

            p += strlen(name) + 1;               
            p = (const char*)align_up(p, 4);     
            depth++;

        } else if (token == FDT_END_NODE) {
            depth --;
        } else if (token == FDT_PROP) {
            // layout: len (4B) + nameoff (4B) + data (len bytes) + padding
            uint32_t len     = read_be32(p);
            uint32_t nameoff = read_be32(p + 4);
            p+=8;
            const char* prop_name = strings_base + nameoff;
            printf("[%4ld] %*s  %s (len=%u)\n",
                    tok_start - struct_base, depth * 2, "", prop_name, len);
            p += len;
            p = (const char*)align_up(p, 4);


        } else if (token == FDT_NOP) {
           
        } else if (token == FDT_END) {
            break;
        } else {
            printf("unknown token 0x%x at %ld\n", token, tok_start - struct_base);
            break;
        }
    }
}


int main() {
    /* Prepare the device tree blob */
    FILE* fp = fopen("qemu.dtb", "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    void* fdt = malloc(sz);
    fseek(fp, 0, SEEK_SET);
    if (fread(fdt, 1, sz, fp) != sz) {
        fprintf(stderr, "Failed to read the device tree blob\n");
        free(fdt);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);

    /* Find the node offset */
    int offset = fdt_path_offset(fdt, "/cpus/cpu@0/interrupt-controller");
    if (offset < 0) {
        fprintf(stderr, "fdt_path_offset\n");
        free(fdt);
        return EXIT_FAILURE;
    }

    /* Get the node property */
    int len;
    const void* prop = fdt_getprop(fdt, offset, "compatible", &len);
    if (!prop) {
        fprintf(stderr, "fdt_getprop\n");
        free(fdt);
        return EXIT_FAILURE;
    }
    printf("compatible: %.*s\n", len, (const char*)prop);

    offset = fdt_path_offset(fdt, "/memory");
    prop = fdt_getprop(fdt, offset, "reg", &len);
    const uint64_t* reg = (const uint64_t*)prop;
    printf("memory: base=0x%lx size=0x%lx\n", bswap64(reg[0]), bswap64(reg[1]));

    offset = fdt_path_offset(fdt, "/chosen");
    prop = fdt_getprop(fdt, offset, "linux,initrd-start", &len);
    const uint64_t* initrd_start = (const uint64_t*)prop;
    printf("initrd-start: 0x%lx\n", bswap64(initrd_start[0]));

    free(fdt);
    return 0;
}

#include <iostream>
#include <list>
#include <vector>

#define NUM_PAGES 0x280000
#define MAX_ORDER 10

struct page {
    int order = 0;
    int refcount = 0;
};

std::vector<page> mem_map;
std::vector<std::list<page*>> free_area;

struct page* get_buddy(struct page* page, unsigned int order) {
    return &mem_map[(page - mem_map.data()) ^ (1 << order)];
}

struct page* alloc_pages(unsigned int order) {
    if (order > MAX_ORDER) return nullptr;

    struct page* free_page = nullptr;
    int free_order = 0;
    for (int i = order; i <= MAX_ORDER; ++i) {
        if (!free_area[i].empty()) {
            free_page = free_area[i].front();
            free_area[i].pop_front();
            free_order = i;
            break;
        }
    }
    if (!free_page) return nullptr;

    for (int i = free_order; i > (int)order; --i) {
        page* buddy_page = get_buddy(free_page, i - 1);
        buddy_page->order = i - 1;
        buddy_page->refcount = 0;
        free_area[i - 1].push_back(buddy_page);
    }

    free_page->order = order;
    free_page->refcount = 1;
    return free_page;
}

void free_pages(struct page* page) {
    if (!page || page->refcount == 0) return;

    page->refcount = 0;
    int current_order = page->order;

    while (current_order < MAX_ORDER) {
        struct page* buddy_page = get_buddy(page, current_order);

        // The order check also rejects untouched interior pages, which look
        // like free order-0 blocks.
        if (buddy_page->refcount != 0 || buddy_page->order != current_order)
            break;

        free_area[current_order].remove(buddy_page);

        if (buddy_page < page) page = buddy_page;  // lower address heads it

        current_order++;
        page->order = current_order;
    }

    free_area[current_order].push_back(page);
}

void dump() {
    for (int i = MAX_ORDER; i >= 0; i--)
        std::cout << "free_area[" << i << "] " << free_area[i].size()
                  << std::endl;
}

int main() {
    mem_map.resize(NUM_PAGES);
    free_area.resize(MAX_ORDER + 1);
    for (size_t i = 0; i < NUM_PAGES; i += (1 << MAX_ORDER)) {
        mem_map[i].order = MAX_ORDER;
        free_area[MAX_ORDER].push_back(&mem_map[i]);
    }

    std::cout << "\np1:\n";
    struct page* p1 = alloc_pages(1);
    dump();

    std::cout << "\np2:\n";
    struct page* p2 = alloc_pages(1);
    dump();

    std::cout << "\np3:\n";
    struct page* p3 = alloc_pages(1);
    dump();

    free_pages(p1);
    free_pages(p2);
    free_pages(p3);

    std::cout << "\nfree:\n";
    dump();
    return 0;
}

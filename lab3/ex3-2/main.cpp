#include <iostream>
#include <list>
#include <vector>

#define PAGE_SIZE (1UL << 12)
#define NUM_PAGES 0x280000
#define MAX_ORDER 10

typedef unsigned long phys_addr_t;

struct page {
    int order = 0;
    int refcount = 0;
};

std::vector<page> mem_map;
std::vector<std::list<page*>> free_area;

struct page* get_buddy(struct page* page, unsigned int order) {
    return &mem_map[(page - mem_map.data()) ^ (1 << order)];
}

void memory_reserve(phys_addr_t base, size_t size) {
    // TODO: Implement this function
    size_t start_pfn = base / PAGE_SIZE;
    size_t end_pfn   = (base + size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int order = MAX_ORDER; order >= 0; --order) {

        auto it = free_area[order].begin();
        while (it != free_area[order].end()) {
            struct page* p = *it;                        
            size_t block_start = p - mem_map.data();     
            size_t block_end   = block_start + (1 << order);
            
            if(block_end <= start_pfn || block_start >= end_pfn) { // no inetrsection
                ++it;
                continue;
            }
            it = free_area[order].erase(it);

            if(block_start>=start_pfn && block_end<=end_pfn) { // block fully inside reserved range
                p->order = order;
                p->refcount = 1;
                continue;
            }

            // block partially overlaps with reserved range, split it
            if(order>0) {
                struct page* buddy = get_buddy(p, order-1);
                p->order = order-1;
                buddy->order = order-1;
                free_area[order-1].push_back(p);
                free_area[order-1].push_back(buddy);
            }

        }
    }
}

void dump() {
    for (int i = MAX_ORDER; i >= 0; i--)
        std::cout << "free_area[" << i << "] " << free_area[i].size()
                  << std::endl;
}

void mm_init() {
    mem_map.resize(NUM_PAGES);
    free_area.resize(MAX_ORDER + 1);
    for (size_t i = 0; i < NUM_PAGES; i += (1 << MAX_ORDER)) {
        mem_map[i].order = MAX_ORDER;
        free_area[MAX_ORDER].push_back(&mem_map[i]);
    }
    memory_reserve(0, 0x82a69510);
}

int main() {
    mm_init();
    dump();
    return 0;
}

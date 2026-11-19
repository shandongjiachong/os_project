#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>
#include <stdio.h>

/*
 * First-Fit contiguous physical page allocator.
 *
 * This module is the reference implementation used in module 2, task 1:
 *   - free_list holds one entry per free block, ordered by page address;
 *   - the head page of a block stores the block length in ->property and
 *     sets PG_property; interior pages keep PG_property = 0;
 *   - nr_free counts the total number of free pages.
 *
 * Allocation walks the list and takes the FIRST block whose length is >= n.
 */

static free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

// insert_free_block - insert one free block into free_list, keeping the list
// ordered by page address so that adjacent blocks can be merged later.
static void
insert_free_block(struct Page *base) {
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *page = le2page(le, page_link);
        if (base < page) {
            list_add_before(le, &(base->page_link));
            return;
        }
    }
    list_add_before(le, &(base->page_link));
}

// default_init - initialize the free list and the free page counter.
static void
default_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

// default_init_memmap - register [base, base + n) as one free block.
static void
default_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    insert_free_block(base);
}

// default_alloc_pages - allocate n contiguous pages with the first-fit rule.
static struct Page *
default_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }

    struct Page *page = NULL;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    if (page != NULL) {
        list_entry_t *prev = list_prev(&(page->page_link));
        list_del(&(page->page_link));
        if (page->property > n) {
            struct Page *rest = page + n;
            rest->flags = 0;
            rest->property = page->property - n;
            SetPageProperty(rest);
            set_page_ref(rest, 0);
            list_add(prev, &(rest->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;
}

// default_free_pages - free n contiguous pages and coalesce free neighbours.
static void
default_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    insert_free_block(base);

    // Merge with the following block if they are physically adjacent.
    list_entry_t *le = list_next(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }

    // Merge with the preceding block if they are physically adjacent.
    le = list_prev(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
        }
    }
}

// default_nr_free_pages - return the number of free pages.
static size_t
default_nr_free_pages(void) {
    return nr_free;
}

// basic_check - exercise alloc/free on single pages.
static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

// default_check - verify the free list bookkeeping and the alloc/free path.
static void
default_check(void) {
    struct Page *p0, *p1;
    list_entry_t *le;

    size_t total = 0;
    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    p0 = alloc_pages(5);
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    p1 = alloc_pages(1);
    assert(p1 != NULL);

    free_pages(p0, 5);
    free_pages(p1, 1);

    total = 0;
    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        total += p->property;
    }
    assert(total == nr_free_pages());
}

const struct pmm_manager default_pmm_manager = {
    .name = "default_pmm_manager",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};


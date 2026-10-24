#include <pmm.h>
#include <list.h>
#include <string.h>
#include <stdio.h>
#include <buddy_pmm.h>

// 学号: 2413551
//
// Buddy System 页面分配器。
// order=k 的块包含 2^k 个连续物理页，块首 Page 的 property 保存块大小(1<<k)，
// PG_property 位标记该 Page 是空闲块头。分配后块头保留 property 供释放时反推 order。

// 最大阶。128MB = 2^15 页，MAX_ORDER 只需覆盖它即可；64 位下 1UL<<20 不会溢出。
#define MAX_ORDER 20

// free_area[o] 管理所有大小为 2^o 页的空闲块；nr_free 记录该阶的空闲块个数。
static free_area_t free_area[MAX_ORDER];
// 所有阶空闲页的总数。
static size_t buddy_nr_free;

// 不变量：buddy_nr_free 恒等于 sum(free_area[o].nr_free * (1<<o), o=0..MAX_ORDER-1)。
// 分裂与合并不改变 buddy_nr_free，分配/释放按实际块容量增减。

// 计算能覆盖 n 页的最小 order（即最小的 k 使 2^k >= n）。
static int
buddy_order_for(size_t n) {
    int order = 0;
    size_t size = 1;
    while (size < n) {
        size <<= 1;
        order++;
    }
    return order;
}

// 判断页 p 是否满足 2^order 的伙伴对齐：起始物理页号的低 order 位全为 0。
static bool
buddy_is_aligned(struct Page *p, int order) {
    return (page2ppn(p) & ((1UL << order) - 1)) == 0;
}

// 返回页 p 在 order 阶下的伙伴块。若伙伴落在管理范围 [nbase, npage) 之外返回 NULL。
static struct Page *
buddy_get_buddy(struct Page *p, int order) {
    ppn_t ppn = page2ppn(p) ^ (1UL << order);
    if (ppn < nbase || ppn >= npage) {
        return NULL;
    }
    return &pages[ppn - nbase];
}

static void
buddy_init(void) {
    for (int i = 0; i < MAX_ORDER; i++) {
        list_init(&free_area[i].free_list);
        free_area[i].nr_free = 0;
    }
    buddy_nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    // 预处理：与 default_init_memmap 一致，清空各页状态与引用计数。
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }

    // 贪心分解：把 [base, base+n) 拆成互不重叠、各自满足阶对齐的 2 的幂块。
    struct Page *cur = base;
    size_t remaining = n;
    while (remaining > 0) {
        int order = MAX_ORDER - 1;
        while (order >= 0) {
            size_t size = 1UL << order;
            if (size <= remaining && buddy_is_aligned(cur, order)) {
                break;
            }
            order--;
        }
        assert(order >= 0);
        size_t size = 1UL << order;
        SetPageProperty(cur);
        cur->property = size;
        list_add(&free_area[order].free_list, &cur->page_link);
        free_area[order].nr_free++;
        buddy_nr_free += size;
        cur += size;
        remaining -= size;
    }
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > buddy_nr_free) {
        return NULL;
    }
    int order = buddy_order_for(n);

    // 找到第一个非空的、阶 >= order 的空闲链表。
    int o = order;
    while (o < MAX_ORDER && list_empty(&free_area[o].free_list)) {
        o++;
    }
    if (o >= MAX_ORDER) {
        return NULL;
    }

    // 取出该阶的一个块。
    list_entry_t *le = list_next(&free_area[o].free_list);
    struct Page *page = le2page(le, page_link);
    list_del(&page->page_link);
    free_area[o].nr_free--;

    // 逐级二分：把多余的高阶块拆成两半，把后半作为伙伴挂入下一阶。
    while (o > order) {
        o--;
        struct Page *buddy = page + (1UL << o);
        SetPageProperty(buddy);
        buddy->property = 1UL << o;
        list_add(&free_area[o].free_list, &buddy->page_link);
        free_area[o].nr_free++;
    }

    ClearPageProperty(page);
    page->property = 1UL << order;  // 保留实际 order，供释放时反推。
    buddy_nr_free -= 1UL << order;
    return page;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    // 由块头保存的元数据恢复实际块容量与 order。
    size_t size = base->property;
    int order = buddy_order_for(size);
    assert(size == (1UL << order));
    assert(size >= n);
    assert(buddy_is_aligned(base, order));

    // 清空被释放页的状态与引用计数。
    struct Page *p = base;
    for (; p != base + size; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }

    buddy_nr_free += size;

    // 向上与真实伙伴合并。
    p = base;
    while (order < MAX_ORDER - 1) {
        struct Page *buddy = buddy_get_buddy(p, order);
        if (buddy == NULL || !PageProperty(buddy) ||
            buddy->property != (1UL << order)) {
            break;
        }
        list_del(&buddy->page_link);
        free_area[order].nr_free--;
        ClearPageProperty(buddy);
        buddy->property = 0;
        if (buddy < p) {
            p = buddy;
        }
        order++;
        size <<= 1;
    }

    SetPageProperty(p);
    p->property = size;
    list_add(&free_area[order].free_list, &p->page_link);
    free_area[order].nr_free++;
}

static size_t
buddy_nr_free_pages(void) {
    return buddy_nr_free;
}

// 学号: 2413551
// ---------------------------------------------------------------------------
// 以下为 buddy_check 的测试辅助函数：仅通过公开接口(alloc_pages/free_pages 等)
// 验证分配器的行为与不变量，不修改 Buddy 算法实现。
// 每个测试函数都做完整的分配-释放往返，结束时状态恢复到测试前。
// ---------------------------------------------------------------------------

// 校验不变量：buddy_nr_free 恒等于所有阶空闲块容量之和，且每个空闲块头
// 的元数据(property/PG_property/对齐)一致。
static void
buddy_check_invariant(void) {
    size_t total = 0;
    for (int i = 0; i < MAX_ORDER; i++) {
        list_entry_t *le = &free_area[i].free_list;
        while ((le = list_next(le)) != &free_area[i].free_list) {
            struct Page *p = le2page(le, page_link);
            assert(PageProperty(p));
            assert(p->property == (1UL << i));
            assert(buddy_is_aligned(p, i));
            total += p->property;
        }
    }
    assert(total == buddy_nr_free);
}

// 场景1：不同大小对象(1,2,3,4,5,8,16页)的分配正确性与块对齐。
static void
buddy_test_alloc_sizes(void) {
    static const size_t sizes[] = {1, 2, 3, 4, 5, 8, 16};
    for (int idx = 0; idx < (int)(sizeof(sizes) / sizeof(sizes[0])); idx++) {
        size_t n = sizes[idx];
        size_t before = buddy_nr_free;
        struct Page *p = alloc_pages(n);
        assert(p != NULL);
        // 分配后块首 PG_property 已清除；property 为覆盖 n 的 2 的幂且 >= n。
        assert(!PageProperty(p));
        int order = buddy_order_for(p->property);
        assert(p->property == (1UL << order));
        assert(p->property >= n);
        // 伙伴对齐：起始页号低 order 位必须为 0。
        assert((page2ppn(p) & ((1UL << order) - 1)) == 0);
        // 实际占用块容量 = property，nr_free 按块容量减少。
        assert(buddy_nr_free == before - p->property);
        free_pages(p, n);
        assert(buddy_nr_free == before);
    }
    buddy_check_invariant();
}

// 场景2：非 2 的幂请求的向上取整(3页->4页、5页->8页)。
static void
buddy_test_roundup(void) {
    size_t before = buddy_nr_free;
    struct Page *p3 = alloc_pages(3);
    assert(p3 != NULL);
    assert(p3->property == 4);
    assert(buddy_nr_free == before - 4);
    free_pages(p3, 3);
    assert(buddy_nr_free == before);

    before = buddy_nr_free;
    struct Page *p5 = alloc_pages(5);
    assert(p5 != NULL);
    assert(p5->property == 8);
    assert(buddy_nr_free == before - 8);
    free_pages(p5, 5);
    assert(buddy_nr_free == before);
    buddy_check_invariant();
}

// 场景3：分裂路径——连续分配两个单页，若 order=0 为空则触发高阶逐级分裂，
// 分裂出的伙伴挂回对应阶链表并可被后续分配复用。
static void
buddy_test_split(void) {
    size_t before = buddy_nr_free;
    struct Page *p0 = alloc_page();
    assert(p0 != NULL);
    assert(buddy_nr_free == before - 1);
    buddy_check_invariant();
    struct Page *p1 = alloc_page();
    assert(p1 != NULL);
    assert(buddy_nr_free == before - 2);
    buddy_check_invariant();
    free_page(p0);
    free_page(p1);
    assert(buddy_nr_free == before);
    buddy_check_invariant();
}

// 场景4：释放与伙伴合并——连续分配两个单页，依次释放后正确合并，
// nr_free 恢复初始值，空闲块容量守恒。
static void
buddy_test_merge(void) {
    size_t before = buddy_nr_free;
    struct Page *p0 = alloc_page();
    struct Page *p1 = alloc_page();
    assert(p0 != NULL && p1 != NULL);
    assert(buddy_nr_free == before - 2);
    free_page(p0);
    assert(buddy_nr_free == before - 1);
    free_page(p1);
    assert(buddy_nr_free == before);
    buddy_check_invariant();
}

// 场景5：计数守恒——混合分配/部分释放/再分配，每一步校验不变量。
static void
buddy_test_mixed(void) {
    size_t before = buddy_nr_free;
    struct Page *p1 = alloc_page();       // 1 页
    struct Page *p3 = alloc_pages(3);     // 实际 4 页
    struct Page *p8 = alloc_pages(8);     // 8 页
    struct Page *p1b = alloc_page();      // 1 页
    assert(p1 && p3 && p8 && p1b);
    assert(buddy_nr_free == before - (1 + 4 + 8 + 1));
    buddy_check_invariant();

    free_pages(p3, 3);                    // 部分释放
    assert(buddy_nr_free == before - (1 + 8 + 1));
    buddy_check_invariant();

    struct Page *p2 = alloc_pages(2);     // 再分配 2 页
    assert(p2 != NULL);
    assert(buddy_nr_free == before - (1 + 8 + 1 + 2));
    buddy_check_invariant();

    free_page(p1);
    free_pages(p8, 8);
    free_page(p1b);
    free_pages(p2, 2);
    assert(buddy_nr_free == before);
    buddy_check_invariant();
}

// 场景6：失败路径——申请超过总空闲页数时返回 NULL 且不改变任何状态。
static void
buddy_test_fail(void) {
    size_t before = buddy_nr_free;
    struct Page *p = alloc_pages(before + 1);
    assert(p == NULL);
    assert(buddy_nr_free == before);
    buddy_check_invariant();

    // 远超物理内存的请求(2^20+1 页)同样应返回 NULL。
    p = alloc_pages((1UL << 20) + 1);
    assert(p == NULL);
    assert(buddy_nr_free == before);
    buddy_check_invariant();
}

// 场景7(说明性)：非法释放——对未分配/非块首/重复释放的页面，buddy_free_pages
// 会因 assert(!PageReserved && !PageProperty)、property 非 2 的幂或块不对齐而
// 触发 panic。此类用例会中断后续测试，故不在此实际执行，仅在实现中由 assert 保证。

// buddy_check：Buddy 分配器完整自检入口，由 pmm_manager->check 调用。
// 依次执行各场景测试，全部通过后恢复到测试前状态。
static void
buddy_check(void) {
    size_t before = buddy_nr_free;
    buddy_check_invariant();

    buddy_test_alloc_sizes();
    buddy_test_roundup();
    buddy_test_split();
    buddy_test_merge();
    buddy_test_mixed();
    buddy_test_fail();

    buddy_check_invariant();
    assert(buddy_nr_free == before);

    cprintf("[Buddy] all checks passed!\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};

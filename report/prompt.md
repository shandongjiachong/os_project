# Lab4 提示词汇总

本文件汇总本实验使用过的 prompt。每个模块统一采用四段式结构：
`[PROMPT]`（做什么、改哪里、怎么改）、`[RELY]`（可依赖的宏/结构/函数）、
`[GUARANTEE]`（必须交付的接口）、`[SPECIFICATION]`（每个函数的行为规格）。

---

## 功能模块一：虚拟内存页操作功能

负责人：2413551-李睿
AI 工具：VS Code 内置 Copilot（底层模型 DeepSeek-V4-Flash）

````
[PROMPT]

**任务**：
修改 `kern/mm/pmm.c`，实现 Lab4「虚拟内存页操作」功能模块。
本模块负责把工程中已有的“页表项定位”能力（`get_pte()`）扩展为“可用的映射管理”
能力：为指定线性地址建立、替换、解除到物理页的映射，并在过程中维护物理页的引用
计数与 TLB 一致性。包含两个函数：
1. `page_insert()`：在给定页目录下把线性地址 `la` 所在虚拟页映射到物理页 `page`，
   并让该 PTE 的权限位等于调用者要求的 `perm`；
2. `page_remove()`：解除线性地址 `la` 所在虚拟页的映射，对其原指向的物理页做引用
   计数回收，且必须在“本来就没有映射”时安静返回（幂等删除接口）。

本模块**不负责**以下内容，请不要越界实现：
- 多级页表本身的查找与页表页分配（`get_pte()` 已实现且不可修改）；
- 缺页异常处理（`do_pgfault()`，本实验未启用）；
- 页表页（PT/PD 页）的回收与释放（`check_pgdir()` 自己负责）；
- 进程地址空间复制（`copy_mm()`）、反向映射、按需分页、页面置换。

**操作要求**：
- 你必须在项目中修改实际文件，而不是只展示代码片段。
- 只在 `kern/mm/pmm.c` 中已有的 `//LAB4 function1 YOUR CODE` 标记位置补全实现，
  保留该注释本身，并把同一行的 `YOUR CODE` 替换为学号 `2413551`。
- 保留文件中其他所有既有代码；不得修改 `get_pte()`、`get_page()`、
  `tlb_invalidate()`、`pmm_init()`、`boot_map_segment()`、`check_pgdir()`、
  `check_boot_pgdir()`、`enable_paging()` 等既有函数。
- 不得修改 `kern/mm/pmm.h` 中的声明、`kern/mm/mmu.h` 中的宏、`struct Page` 布局，
  也不得修改课程测试与评分脚本。
- 允许新增仅在本模块内部使用的 `static` 辅助函数与内部宏；不得新增全局变量，
  不得改变任何公开接口。

**输出要求**：
直接进行文件操作。使用下面 `[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 中的信息；
一切以工程中真实存在的宏名、字段名和函数签名为准（例如页大小宏是 `PGSIZE`，
不是 `PAGE_SIZE`），不要凭记忆引入不存在的宏或自行改写数据布局，不要引入与本模块
无关的功能。


[RELY]

// 1) 页表项定位（既有实现，不可修改）
pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create);
// - pgdir 为页目录（Sv39 三级中的最高层）的内核虚拟地址；
// - create != 0 时，缺失的中间层/末级页表页会被 alloc_page() 分配并清零，
//   分配失败时返回 NULL；create == 0 且缺表时同样返回 NULL；
// - 成功返回该线性地址对应 PTE 的内核虚拟地址（可直接读写 *ptep）；
// - 既有实现用 PTE_U | PTE_V 建立中间层表项，并用 set_page_ref(..., 1) 记录表页
//   的引用计数，因此“页表页”的引用计数不属于本模块的管理范围。
struct Page *get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store);

// 2) PTE 的构造与解析
static inline pte_t pte_create(uintptr_t ppn, int type)   // (ppn << PTE_PPN_SHIFT) | PTE_V | type
static inline pte_t ptd_create(uintptr_t ppn)             // pte_create(ppn, PTE_V)
#define PTE_ADDR(pte) (((uintptr_t)(pte) & ~0x3FF) << (PTXSHIFT - PTE_PPN_SHIFT))
#define PDE_ADDR(pde) PTE_ADDR(pde)
static inline struct Page *pte2page(pte_t pte);  // 要求 PTE_V 已置位，否则 panic
// 注意：PTE_V 由 pte_create() 自动置位，perm 只表达调用者关心的权限位；
//       PTE_PPN_SHIFT == 10，PGSHIFT == 12，PGSIZE == 4096。

// 3) 页与地址换算
page2ppn(page)  ->  page - pages + nbase
page2pa(page)   ->  page2ppn(page) << PGSHIFT
pa2page(pa);  page2kva(page);
#define PPN(la)   (((uintptr_t)(la)) >> PTXSHIFT)
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// 4) 物理页分配与引用计数
#define alloc_page()  alloc_pages(1)
#define free_page(page) free_pages(page, 1)
page_ref(page);  set_page_ref(page, val);  page_ref_inc(page);  page_ref_dec(page);

// 5) 权限位（kern/mm/mmu.h）
PTE_V 0x001  PTE_R 0x002  PTE_W 0x004  PTE_X 0x008  PTE_U 0x010
// 页目录/页表的全局变量
extern pde_t *boot_pgdir_va;   // 内核页目录的内核虚拟地址，pmm_init() 之后可用

// 6) TLB 维护
void tlb_invalidate(pde_t *pgdir, uintptr_t la);   // 既有实现：sfence.vma la
static inline void flush_tlb(void);                // sfence.vma（全量）

// 7) 错误码（libs/error.h）
#define E_NO_MEM 4      // 函数应以 -E_NO_MEM 形式返回失败

// 8) 可验证的验收背景（用于校准行为，不需要你编写测试）
// a) pmm.c::check_pgdir()（pmm_init() 内部调用，每次启动都会执行）
//    page_insert(boot_pgdir_va, p1, 0x0, 0)               -> 0，page_ref(p1) == 1，
//      get_pte(..., 0x0, 0) 非空且 pte2page(*ptep) == p1
//    page_insert(boot_pgdir_va, p2, PGSIZE, PTE_U|PTE_W)  -> 0，
//      *ptep 同时带 PTE_U 与 PTE_W，page_ref(p2) == 1
//    page_insert(boot_pgdir_va, p1, PGSIZE, 0)            -> 0，
//      page_ref(p1) == 2（同一物理页被映射两次，新页引用递增而非置 1），
//      page_ref(p2) == 0（被覆盖的旧页解除映射后释放），
//      *ptep 指向 p1 且 (*ptep & PTE_U) == 0（权限被整体替换为 perm == 0）
//    page_remove(boot_pgdir_va, 0x0)                       -> page_ref(p1) == 1
//    page_remove(boot_pgdir_va, PGSIZE)                    -> page_ref(p1) == 0
//      两次 remove 之后 page_ref(pde2page(boot_pgdir_va[0])) 仍为 1，
//      且函数返回时 nr_free_pages() 与进入前一致（不泄漏、不误放页表页）
// b) pmm.c::check_boot_pgdir()
//    page_insert(boot_pgdir_va, p, 0x100, PTE_W|PTE_R)    // la 未按页对齐
//      写 0x100 的内容可在 0x100 + PGSIZE 读到（同一物理页映射到两个虚拟页，
//      page_ref(p) == 2），说明 la 的页内偏移被忽略、映射粒度为页
// c) tools/test_lab4.c::test_lab4_pmm_basic()（grade.sh 的 'check PMM'，10 分，
//    通过标志为输出 '[Test PMM] PASS'）
//    page_insert(p3, 2*PGSIZE, 0) 之后再 page_insert(p3, 2*PGSIZE, PTE_W)
//      -> page_ref(p3) == 1 且 nr_free_pages() 不变（同页重映射不得增引用，
//         也不得把该页当成“旧页”释放后重新分配）
//      -> 注意：测试只断言了引用计数与空闲页数，权限是否更新为本模块的行为规格
//         要求（权限位必须等于 perm），由验收时人工核对
//    page_remove 连续两次作用于同一地址不得崩溃、不得产生副作用


[GUARANTEE]

必须实现或修改的接口（均定义在 `kern/mm/pmm.c`，签名必须与 `kern/mm/pmm.h`
中的声明完全一致）：

```c
int  page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);
void page_remove(pde_t *pgdir, uintptr_t la);
```

你可以根据需要添加：
- 仅在本模块内部使用的 `static` 辅助函数（例如统一的“解除单条 PTE 映射”步骤）
- 内部使用的宏定义
- 模块内部的静态变量

不允许：
- 新增或修改任何全局变量、公开函数签名、头文件声明
- 修改 `get_pte()` / `tlb_invalidate()` / `check_pgdir()` 等既有实现
- 实现 `do_pgfault()`、`pgdir_alloc_page()` 等属于后续实验的功能
  （`pmm.h` 中已声明 `pgdir_alloc_page()` 但工程中暂无调用者，本模块不要求实现，
  也不得删除其声明）


[SPECIFICATION]

## page_insert

**Pre-Condition**：
- `pgdir` 是有效的页目录内核虚拟地址（如 `boot_pgdir_va`）。
- `page` 来自 `alloc_page()` / `alloc_pages()`，指向一个可映射的物理页；不得是页表页、
  已释放页或 `NULL`。
- `la` 是任意线性地址，**不要求页对齐**；其页内偏移 `PGOFF(la)` 不参与映射。
- `perm` 只包含调用者要求的权限位（`PTE_U` / `PTE_W` / `PTE_R` / `PTE_X` 的组合），
  有效位 `PTE_V` 由页表项构造过程统一置位。

**Post-Condition**：
- `la` 所在虚拟页的页表项有效（`PTE_V` 置位），且指向 `page` 对应的物理页。
- 该页表项的权限位等于 `perm`（不额外附加调用者未要求的权限，也不遗漏要求的权限）。
- `page` 的引用计数准确反映“当前有多少个虚拟页映射到它”：新增一条映射使其加一，
  覆盖掉一条指向它的旧映射则不加，重复映射同一位置则不变。
- 该虚拟页的 TLB 缓存已失效，后续访问能看到新的映射与权限。
- 函数返回 0 表示成功。

**Case 1**（该位置原本没有有效映射）：
为新映射取得页表项；若成功，填入指向 `page` 的表项并把该物理页的引用计数加一，
使该虚拟页可被访问，返回 0。

**Case 2**（该位置已经映射到同一个 `page`）：
这是一次“重映射/改权限”，只更新该页表项的权限位，**不得改变** `page` 的引用计数，
也不得释放或重新分配该页；仍须使该虚拟页的 TLB 缓存失效，返回 0。
（若先把旧映射减引用再重新加引用，引用计数会短暂归零并触发释放，属于错误实现。）

**Case 3**（该位置已经映射到另一个物理页）：
先解除旧映射：旧页引用计数减一，减到 0 时释放该旧页；再按 Case 1 建立到 `page` 的
新映射（`page` 的引用计数加一）。此路径结束后只应存在一条指向 `page` 的有效映射，
旧页不再被任何映射引用。返回 0。

**Case 4**（页表页分配失败）：
为 `la` 获取页表项时若因内存不足失败，必须返回 `-E_NO_MEM`；此时**不得**写入任何
页表项、**不得**修改 `page` 的引用计数、**不得**破坏该位置上原有的映射或权限。

**Requirements**：
- 任何修改页表项的路径都必须在写完之后使目标虚拟页的 TLB 缓存失效，写之前刷 TLB
  视为无效实现（虽然某些硬件上可能“碰巧”能用）。
- 成功路径与失败路径都必须保持“引用计数与有效映射条数一致”这一不变量；
  不允许出现“表项已改而引用未改”或“引用已改而表项未改”的半完成状态。
- 不得释放或修改页表页（中间层页）以及 `pgdir` 自身。
- 不得在函数内分配除页表页之外的物理页（`alloc_page()` 的调用只应通过既有
  `get_pte()` 间接发生）。

## page_remove

**Pre-Condition**：
- `pgdir` 是有效的页目录内核虚拟地址。
- `la` 是任意线性地址，不要求页对齐。

**Post-Condition**：
- 若 `la` 所在虚拟页存在有效映射：该页表项被清除为无效，原物理页引用计数减一，
  减到 0 时该物理页被释放；该虚拟页的 TLB 缓存已失效。
- 若不存在有效映射：函数不产生任何状态变化，正常返回（不 panic、不打印致命错误、
  不修改任何页表项或引用计数）。
- 函数无返回值，重复调用结果一致（幂等）。

**Case 1**（存在有效映射）：
定位该位置的页表项，取得其指向的物理页，清除该表项（使之不再有效），把该物理页的
引用计数减一；引用计数归零时释放该物理页；最后使该虚拟页的 TLB 缓存失效。

**Case 2**（该地址从未映射过，或其页表页不存在）：
以“不创建页表页”的方式定位页表项；一旦定位失败或表项无效，立即安静返回，
不得把“地址未映射”当成异常情况处理。

**Case 3**（同一地址连续调用两次）：
第一次调用完成 Case 1 的清理；第二次调用走 Case 2 的安静返回，不得对已释放的物理页
再次减引用或重复释放。

**Requirements**：
- 只能释放被解除映射的那一个数据页，不得释放页表页，也不得修改 `pgdir`。
- 清除表项、修改引用计数、失效 TLB 三步的顺序必须保证不会被中断之外的观察者看到
  不一致的中间状态（本实验为单核、且调用点通常已关中断，至少要求逻辑顺序正确）。
- 不得因为参数 `la` 未页对齐而 panic 或算错目标页。

## 端到端组合行为

**Pre-Condition**：`pmm_init()` 已完成，`boot_pgdir_va` 可用，物理内存管理器能正常
分配与回收页。

**Post-Condition**：
`page_insert()` 与 `page_remove()` 构成一对互逆操作：对同一 `la` 先 insert 再 remove
之后，该物理页的引用计数回到 insert 之前的值、页表项不再有效、`nr_free_pages()` 不
减少；连续多次 insert/remove 不应出现引用计数漂移或物理页泄漏。该组合行为同时满足
`check_pgdir()`、`check_boot_pgdir()` 与 `test_lab4_pmm_basic()` 中的断言。
````

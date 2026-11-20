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

---

## 功能模块三：进程调度与上下文切换

负责人为成钧翔，学号 2411259，使用的 AI 工具为 Codex。

以下四段式提示词根据本次实际实现和联调过程整理。任务围绕“从已经创建的线程中选择运行对象，并让它获得 CPU”展开，覆盖调度请求、线程选择、页表切换、寄存器切换和首次运行衔接。

````markdown
[PROMPT]

请基于当前 Lab4 工程完成进程调度与上下文切换模块。先读取实际源码，
确认前两个模块已经提供的进程状态、链表、页表和首次运行上下文，再实现
以下完整调用链，并在 WSL 中运行现有课程测试。

cpu_idle() → schedule() → proc_run(next) → switch_to(from, to)

任务目标

- idleproc 发现调度请求后能调用调度器。
- 调度器能够从 proc_list 中选择 PROC_RUNNABLE 线程，遍历一圈结束后
  能确定运行对象，找不到可运行线程时选择 idleproc。
- 线程切换期间，current、satp 和实际寄存器现场的更新受到本地中断保护。
- 第一次切换到 initproc 时，能沿已有恢复链进入 init_main()。
- 已经运行过的线程再次获得 CPU 时，能够从上一次切出的调用位置继续。

修改范围

- kern/process/proc.c 的 LAB4 function3 区域，实现 cpu_idle()、proc_run()。
- kern/schedule/sched.c 的 LAB4 function3 区域，实现 schedule()。
- kern/process/switch.S 的 LAB4 function3 区域，实现 switch_to()。
- 保留 LAB4 function3 标记，将 YOUR CODE 替换为 2411259。
- 按实际使用情况补充必要头文件，复用现有中断保护和 TLB 刷新接口。
- 保持 proc_struct、context、trapframe 的布局、公开函数签名和已有宏定义。
- 保持前两个模块的资源分配、线程登记、线程初始现场和唤醒约定。
- 保持课程测试、评分脚本和框架提供的线程入口及 Trap 返回流程。
- 原有 do_exit() 的行为作为运行现象记录，本模块交付范围为上述四个函数。

依赖检查与交付要求

开始前检查 alloc_proc()、proc_init()、kernel_thread()、do_fork()、
copy_thread() 是否已有实现，并确认 proc->pgdir 的地址含义。
联调时核对 PCB 初始状态、就绪链表和新线程首次运行现场，确认创建结果符合
调度接口的前置条件，并根据实际输出记录集成验证结果。

完成后说明修改文件、关键分支、实际运行命令和测试结果。测试失败时根据
编译输出或运行现场定位原因。保留完整日志，并区分源码行为问题和工具链
兼容问题。此次工作先保存在本地，提交和推送按后续明确指令执行。


[RELY]

只使用当前工程实现这条调用链所需的接口和布局。

1. 线程管理数据

struct proc_struct 中需要使用的成员为
state、runs、need_resched、context、pgdir、list_link。

extern struct proc_struct *current;
extern struct proc_struct *idleproc;
extern struct proc_struct *initproc;
extern list_entry_t proc_list;

PROC_RUNNABLE 同时覆盖可运行和正在运行的状态。
idleproc 为调度器的兜底线程，普通线程通过 list_link 登记到 proc_list。
proc_list 自身是哨兵节点，只有实际线程节点可以用 le2proc() 转回 PCB。
list_next()、le2proc() 已在框架中提供。

2. 寄存器上下文

struct context 的字段顺序是
ra、sp、s0、s1、s2、s3、s4、s5、s6、s7、s8、s9、s10、s11。

它们都是 uintptr_t。RV64 下共 14 个槽，每槽 8 字节，总计 112 字节。
switch_to() 的 a0 参数指向旧 context，a1 参数指向新 context。
按 RISC-V 函数调用约定，保留返回地址、栈指针和被调用者保存寄存器。
复用 riscv.h 提供的 REGBYTES、STORE 和 LOAD。

3. 中断保护

local_intr_save(flag) 保存原 SIE 状态并屏蔽本地 S 级中断。
local_intr_restore(flag) 根据保存值恢复，支持配对使用的嵌套临界区。
schedule() 和 proc_run() 分别保存自己的 flag。

4. 页表接口

proc->pgdir 保存页表根的物理地址，当前内核线程共享 boot_pgdir_pa。
RV64 Sv39 的 satp.MODE 为 8，位于 bit 63–60，根页表地址按物理页号写入。
使用 SATP_MODE_SV39、RISCV_PGSHIFT、write_csr(satp, value) 和 flush_tlb()。
核对 kern/mm/pmm.c 的 enable_paging() 所使用的编码。
libs/riscv.h 的 lsatp(unsigned int) 使用 Sv32 格式，本模块使用 RV64 Sv39 编码。

5. 新线程首次运行约定

前面的 copy_thread() 已把 context.ra 设置为 forkret，context.sp 指向
新线程内核栈中的 TrapFrame，帧内 epc 指向 kernel_thread_entry。
首次运行链为
forkret → forkrets → __trapret → sret → kernel_thread_entry → 线程函数。

forkret、forkrets、__trapret、kernel_thread_entry 和 TrapFrame 恢复代码
已经由框架和前序模块提供，本模块按既定 context 布局接入。

6. 现有验证入口

make qemu 运行普通内核，init_main() 输出当前线程身份与 Hello world!!。
make grade 在临时工程中注入课程测试，检查 PMM、Process Create 和 Schedule。
tools/test_lab4.c 的调度测试会创建两个额外线程，让它们主动调用 schedule()，
检查两个线程都曾实际执行。


[GUARANTEE]

必须交付以下四个接口，签名与现有声明一致。

void cpu_idle(void);
void schedule(void);
void proc_run(struct proc_struct *proc);
void switch_to(struct context *from, struct context *to);

整体行为承诺

- cpu_idle() 持续运行，检测到 need_resched 时触发调度。
- schedule() 在一次有界查找中确定目标，遵守线程可运行状态和 idle 兜底规则。
- proc_run() 让 current、页表根和寄存器上下文与被选择的线程一致。
- switch_to() 完整保存、恢复 context 约定的 14 个寄存器槽。
- 调度与线程切换的关键区域使用现有本地中断保护接口。
- 新线程能够首次进入函数，暂停的线程能够恢复到先前的调用位置。
- 原有公开接口、结构体布局和前序线程创建约定保持一致。


[SPECIFICATION]

一、cpu_idle()

前置状态

- proc_init() 已完成，current 指向 idleproc，相关链表已初始化。
- 启动代码已经准备好时钟与中断环境。

行为与可核对结果

- 保持空闲循环，与声明中的 noreturn 约定一致。
- current->need_resched 为真时调用 schedule()。
- 标志为假时继续等待。schedule() 返回后重新检查标志。
- 线程选择由 schedule() 完成。cpu_idle() 通过该接口交出 CPU。

二、schedule()

前置状态

- current 和 idleproc 有效，proc_list 是合法循环双向链表。
- 普通线程的 list_link 已正确登记，可运行线程标记为 PROC_RUNNABLE。

关键行为

- 保存进入前的中断状态并屏蔽本地中断，然后清除当前线程的 need_resched。
- 当前线程是 idleproc 时，从 proc_list 的第一个实际节点开始选择。
- 当前线程是普通线程时，从它的下一个节点开始选择，遇到链表尾可回到前部。
- 扫描时跳过 proc_list 哨兵，只有实际线程节点参与状态判断。
- 选择查找顺序中第一个 PROC_RUNNABLE 线程，完整扫描一圈后结束查找。
- 所有普通线程都不可运行时，选择 idleproc。
- 所选线程的 runs 增加一次，调用 proc_run(next) 完成必要的切换。
- 此次调用恢复执行并返回时，使用本层保存的 flag 恢复中断状态。

分支场景

- 链表为空时能选择 idleproc，查找过程有限结束。
- 当前为 idleproc 且 initproc 就绪时，能选择 initproc。
- 有多个就绪线程时，从当前节点之后查找，给其他就绪线程运行机会。
- 当前普通线程是唯一可运行线程时，一圈查找能再次选择它并继续执行。
- 查找到睡眠或僵尸线程时跳过该节点。
- next 等于 current 时照常统计本次选择，proc_run() 保持当前执行状态。

三、proc_run(proc)

前置状态

- proc 是调度器选出的有效目标线程，其上下文已经准备好。
- proc->pgdir 是页对齐的根页表物理地址，页表包含可执行内核及线程栈的映射。

分支场景

- proc 等于 current 时直接完成调用，保留当前页表和寄存器上下文。
- proc 与 current 不同时，在保存并屏蔽本地中断的范围内记住旧线程指针，
  更新 current，装入目标页表，完成 TLB 同步并调用 switch_to()。
- 使用旧线程的 context 作为 from，目标线程的 context 作为 to。
- satp 的 MODE 编码为 ((uintptr_t)SATP_MODE_SV39 << 60)，
  PPN 部分为 proc->pgdir >> RISCV_PGSHIFT，按位组合后写入 satp。
- satp 写入后调用 flush_tlb()，随后切换寄存器现场。

暂停与恢复语义

switch_to() 可能切到另一条执行流。旧线程的 proc_run() 调用会暂停，等旧线程
以后重新获得 CPU，才会从 switch_to() 后继续。此时使用旧线程栈上保留的 flag
恢复本层原来的中断状态。新线程首次运行时，通过预先构造的 TrapFrame 和 sret
恢复到该线程指定的权限与中断状态。

四、switch_to(from, to)

前置状态

- from 和 to 指向可读写且布局正确的 struct context。
- 调用者已在安全的临界区内做好目标线程和页表的切换准备。

布局与恢复效果

| 字段 | 槽号 | RV64 字节偏移 |
| --- | --- | --- |
| ra | 0 | 0 |
| sp | 1 | 8 |
| s0–s11 | 2–13 | 16–104 |

- 将旧线程的 ra、sp、s0–s11 保存到 from 对应槽位。
- 从 to 的对应槽位恢复这 14 个寄存器，寻址始终使用正确的 context 指针。
- 恢复后执行 ret，根据目标 ra 转移，目标 sp 决定后续使用的线程栈。
- 首次运行的线程从预设的 forkret 开始，已经运行过的线程继续其先前调用。
- 函数仅承担调用约定要求的现场迁移，调度选择和 satp 更新由上层完成。

五、验证和输出要求

- 先编译当前完整工程，确认四个接口均参与正常链接。
- 用 make qemu 验证能出现 initproc 身份、Hello world!! 和后续线程输出。
- 线程函数返回后，当前框架的 do_exit() 会触发 process exit!!. 的 panic
  并进入调试监视器。保留原始输出并说明这一现象的来源。
- 用 make grade 验证 PMM、Process Create、Schedule 三项，记录原始总分。
- 遇到工具链兼容诊断时记录编译器版本、错误位置和实际使用的参数。
- 对照上面的分支场景检查代码，说明现有自动测试覆盖了哪些行为，以及哪些
  边界只进行了人工代码检查。
````

### 实际联调记录

本次使用 WSL、RISC-V GCC 15.1.0 和 QEMU。初次编译遇到框架 `kmalloc.c` 中整数
地址传给指针参数的诊断，评分编译还遇到测试线程返回类型与 `kernel_thread()`
参数类型不匹配的诊断。验证时使用以下两个编译参数将对应诊断降为警告。

```bash
cd /mnt/d/os/lab4_publish/code
export PATH="/opt/riscv-elf-toolchains/bin:/home/lab/tools/qemu-4.1.1/riscv64-softmmu:$PATH"
make qemu DEFS="-Wno-error=int-conversion -Wno-error=incompatible-pointer-types"
make grade DEFS="-Wno-error=int-conversion -Wno-error=incompatible-pointer-types" GRADE_TMP="$(mktemp -d /tmp/lab4-grade.XXXXXX)"
```

普通启动成功进入 PID 为 1、名为 `init` 的线程，打印 `Hello world!!`，随后进入
框架 `do_exit()` 提供的调试监视器。课程评分的 PMM、Process Create、Schedule
三项均为 OK，总分为 `30/30`。调度测试实际检查了两个额外线程都曾运行，并经历
多次主动调度。空链表、仅当前线程可运行、全无就绪线程等分支按源码进行了人工
检查，本次未将它们记录为单独执行过的自动测试。

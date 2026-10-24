# Lab2 提示词汇总

本文件汇总 A 的物理内存初始化提示词、王泊凯（2412224）的 Best-Fit 提示记录与李睿（2413551）的 Buddy System 两份提示词。

# 任务一：物理内存探测与页描述符初始化

负责人：2411259-成钧翔（A）。

## 提示词设计

我先区分物理地址、内核虚拟地址、物理页号和 Page 描述符地址，理解哪些区域必须保留，再明确 `page_init()` 与分配器的职责边界。在此基础上，我按 Prompt、Rely、Guarantee、Specification 四部分编写提示词，描述目标、依赖、责任和可验证行为，而不是用一段伪代码代替需求分析。

## 完整提示词

````markdown
请在当前 uCore Lab2 项目中实现 kern/mm/pmm.c 的 page_init()，并在 WSL 中实际编译、运行和使用 GDB 验证。

[Prompt]
根据 DTB 返回的物理内存起点和大小，建立可管理物理页的 struct Page 描述符布局，并将排除 OpenSBI、内核镜像和描述符数组占用空间后的完整空闲页面交给当前 pmm_manager。仅修改 page_init() 对应的 lab2 实现区域，不修改测试、公共结构或其他分配器。

[Rely]
只依赖 get_memory_base()、get_memory_size()、panic()、cprintf()、init_memmap()、pa2page()、链接符号 end、struct Page，以及现有的 DRAM_BASE、PHYSICAL_MEMORY_END、PHYSICAL_MEMORY_OFFSET、PADDR、PPN、ROUNDUP、ROUNDDOWN 和页面状态宏。

[Guarantee]
正确设置 va_pa_offset、npage 和 pages；初始化所有 Page；保持内核与描述符数组占用页面为 reserved；仅把合法完整空闲页注册给分配器；保留原有接口和工程语义。

[Specification]
检查 DTB 范围为非零且加法不溢出；确认内存起点符合平台布局；管理上界不得超过框架上限；pages 必须页对齐并位于内核 end 之后；检查描述符数组大小计算和地址加法是否溢出，并核验实际运行中的数组布局；free_begin 必须位于描述符数组之后并向上页对齐；只有 free_begin < free_end 时调用 init_memmap()。完成后执行现有页面初始化测试，并用 GDB 检查关键全局变量和 reserved 状态。
````

## 对提示词结果的检查

重点检查 `npage` 与描述符数量的区别、地址转换和页对齐、保留页状态、描述符数组占用范围，以及登记前后的空闲页计数。运行截图和具体分析见 [实验报告 A 部分](./实验报告模板.md)。

---

# 任务二：Best-Fit

姓名：王泊凯；学号：2412224。以下区分真实发送记录、预写分工提示词和事后说明，不把它们混为同一回事。实际提示词及必要说明直接收录于本文件，无需打开旧材料目录。

## 1. 首次实际用户 Prompt

以下为本会话事件 seq 9 的完整原文。原分工中的预写提示不是每一轮实际对话的替代品；引文中的本机路径只记录当时输入，不是本文件的附件依赖。

````text
@C:\Users\18504\xwechat_files\wxid_gmxjuz1cbaju22_f8cb\msg\file\2026-10\新建文件夹.zip 读一下文件内容，这是实验二给我们小组三人的分工，里面好像提示词已经写好了，这是实验指导书http://oslab.mobisys.top/lab2026/_book/ ，@D:\lab2.zip 这是实验二的代码，使用子智能体deepseek v4.1 flash max
````

## 2. 最初实现任务及实际执行者

以下记录来自实际工具调用 seq 87，保留原记录的转义反引号。这是一次尝试派发的实现任务，不是成功调用的证明。

````text
Historical tool dispatch seq 87. This records an attempted delegation, not proof of successful execution or actual model.

You are implementing the Best-Fit physical memory allocator for uCore Lab2 (RISC-V).

## Working directory
C:/Users/18504/Desktop/OS/_tmp_code/lab2_new

## Task
Implement Best-Fit in \`kern/mm/best_fit_pmm.c\` and switch pmm in \`kern/mm/pmm.c\`.

## Files already read — key facts

### kern/mm/best_fit_pmm.c (current state, 138 lines)
The file has the check functions already (basic_check, best_fit_check) but the implementation functions are missing — there are only stubs. You must add all the implementation. The structure uses the single free_area_t pattern (one free list, ordered by physical address ascending).

Key invariants from best_fit_check():
- When freeing p0+1(2 pages) and p0+4(1 page), then alloc_pages(4) must return NULL
- alloc_pages(1) must select p0+4 (the 1-page block, smallest fit ≥ 1) NOT p0+1
- alloc_pages(2) must then succeed from the p0+1..p0+2 block

### kern/mm/default_pmm.c (complete, 254 lines)
This is First-Fit. Use it as a structural template. Key difference: in alloc, First-Fit takes the FIRST block with property ≥ n; Best-Fit must scan ALL blocks and pick the one with MINIMUM property ≥ n.

### struct Page (from memlayout.h):
\`\`\`c
struct Page {
    int ref;
    uint64_t flags;
    unsigned int property;   // for head of free block: size of block
    list_entry_t page_link;
};
\`\`\`

Macros: SetPageProperty, ClearPageProperty, PageProperty, SetPageReserved, ClearPageReserved, PageReserved

free_area_t has: list_entry_t free_list, unsigned int nr_free

le2page(le, page_link) converts list_entry to Page*

list ops: list_init, list_add (add after head), list_add_before(le, new), list_next, list_prev, list_empty, list_del

### kern/mm/pmm.c (91 lines)
init_pmm_manager() currently sets: pmm_manager = &default_pmm_manager;
You must change it to: pmm_manager = &best_fit_pmm_manager;

## What you must write

### best_fit_pmm.c — full implementation

Add at the top (after includes, before basic_check):
\`\`\`c
static free_area_t free_area;
#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)
\`\`\`

Implement these 5 functions (add them before or after the check functions, but the check functions must NOT be modified):

1. **best_fit_init** — same as default_init: init free_list, set nr_free=0

2. **best_fit_init_memmap(struct Page *base, size_t n)** — same logic as default_init_memmap:
   - clear all pages in [base, base+n)
   - set base->property = n, SetPageProperty(base), nr_free += n
   - insert base into free_list in address-ascending order

3. **best_fit_alloc_pages(size_t n)** — DIFFERENT from First-Fit:
   - if n > nr_free return NULL
   - scan ALL entries in free_list, find the one with MINIMUM property >= n (best fit)
   - if found: remove it, split if property > n (remainder goes back into list in address order), ClearPageProperty on allocated head, nr_free -= n, return page
   - if not found: return NULL

4. **best_fit_free_pages(struct Page *base, size_t n)** — same as default_free_pages:
   - clear all pages in range, set base->property=n, SetPageProperty(base), nr_free += n
   - insert in address-ascending order
   - merge with previous block if adjacent
   - merge with next block if adjacent

5. **best_fit_nr_free_pages** — return nr_free

6. **best_fit_check** — already exists, do NOT modify it

7. Add at the end:
\`\`\`c
const struct pmm_manager best_fit_pmm_manager = {
    .name = "best_fit_pmm_manager",
    .init = best_fit_init,
    .init_memmap = best_fit_init_memmap,
    .alloc_pages = best_fit_alloc_pages,
    .free_pages = best_fit_free_pages,
    .nr_free_pages = best_fit_nr_free_pages,
    .check = best_fit_check,
};
\`\`\`

### pmm.c
Change only ONE line in init_pmm_manager():
  FROM: pmm_manager = &default_pmm_manager;
  TO:   pmm_manager = &best_fit_pmm_manager;

## Critical correctness notes for best_fit_alloc_pages

The check does this:
\`\`\`
free_pages(p0 + 1, 2);  // block at p0+1, size 2
free_pages(p0 + 4, 1);  // block at p0+4, size 1
// free_list: [p0+1(2), p0+4(1)]  (ascending order)
assert(alloc_pages(4) == NULL);          // no block >= 4
assert(p0[1].property == 2);             // block at p0+1 still has property 2
p1 = alloc_pages(1);                     // best fit for 1: pick p0+4 (size 1), NOT p0+1 (size 2)
assert(p0 + 4 == p1);                    // must be p0+4
assert(alloc_pages(2) != NULL);          // now free_list: [p0+1(2)], alloc 2 succeeds
\`\`\`

So best-fit MUST scan entire list and pick the minimum-size block that fits.

## Steps

1. Read the current best_fit_pmm.c to see exact content
2. Write the complete new best_fit_pmm.c with all implementations
3. Edit pmm.c to switch the manager
4. Verify both files look correct

Use the read and write/edit tools to accomplish this. Write the COMPLETE file for best_fit_pmm.c (not just a patch), preserving the existing check functions exactly.
````

该次请求使用 deepseek/deepseek-v3-250324 路由，返回错误：

> child LLM route "deepseek/deepseek-v3-250324" is not allowed for this Session

这既不是用户指定的 Flash 路由，也没有成功执行。随后源码由主助手写入（seq 108；管理器选择 seq 122），不能写成“DeepSeek Flash 首次生成并通过验证”。该历史提示词的文件状态与路径描述对应当时快照，不作为当前仓库状态说明。

## 3. 后续实际约束与最终工作 Prompt

### 3.1 身份与分支要求原文

````text
姓名：王泊凯 学号：2412224，github可以直接提交到Lab2分支，不用个人分支，仍旧是给我命令，你不执行
````

这条消息当时要求只提供命令；其后的执行请求另行授权了本地验证和材料准备。

### 3.2 本地验证与材料准备的执行 Prompt 原文

````text
你是操作系统实验助手。请依据已有流程、实验指导书和小组分工，直接执行我负责的 Lab2 Best-Fit 部分的验证（即B同学），并补齐全部提交材料（含截图），整理到仅需由我自行提交的状态。姓名：王泊凯；学号：2412224。不要只提供操作命令。

### 执行步骤
1. **读取并核对资料**：可读取项目目录 `/mnt/c/Users/18504/Desktop/OS/os_project` 和本地实现目录 `/mnt/c/Users/18504/Desktop/OS/_tmp_code/lab2_new`。查阅实验指导书、分工及现有材料，确认代码、工作树和 `Lab2` 分支状态；找不到的资料或路径须明确说明，不猜测要求。
2. **检查负责范围**：沿用已有独立工作目录流程，不创建个人分支，不删除或强制覆盖已有工作。核对 Best-Fit 实现及管理器接入语句，保留原有两个测试函数、队友的 `page_init()` 和学号标记。若目标代码已被队友更新，先比较，不直接覆盖。
3. **执行真实验证**：核实工具链和 QEMU 环境，依次执行编译、普通自检和正式 `make grade`。已知本机 QEMU 7.0.0 曾存在固件与评分启动方式兼容问题，不得假定更换版本即可通过，也不得将额外启动配置改动混入本次材料。超时退出不等于验证通过。
4. **补齐提交材料**：按指导书和分工要求整理代码及所需文档，正确填写姓名、学号；在 `report/B-best-fit/evidence` 下保存基础提交号、受测代码差异、环境版本、执行命令、完整日志、退出码及真实截图。截图须来自实际运行；无法截图时明确列为缺项，不伪造或用文字冒充。
5. **复核并交接**：依据同一次正式评分运行的日志和退出码判断结果，核对 Page Init、Best-Fit 检查及 `Total Score: 20/20`。逐项检查提交材料是否齐全，最后列出材料路径、验证结论、缺失信息和阻塞项。

### 必须遵守
- 正式评分未通过，不得写成“已验收”；区分编译成功、自检通过和正式评分通过。
- 所有结论必须有真实证据，不编造结果；存在阻塞时完成可安全执行的部分，并说明剩余事项。
- 仅整理本地材料供我提交，不执行 Git 提交、推送或任何代提交操作。
````

引文中的旧材料保存位置保留为历史记录，不要求读者打开该目录。禁止 commit/push 是当时对助手的权限约束，不代表后来用户自行提交没有发生。

### 3.3 只读审查 Prompt 原文

````text
请用中文只读审查 B Best-Fit 本地交付副本，不要改任何文件、不构建、不运行QEMU。用户王泊凯2412224，只授权本地准备，禁止commit/push。工作目录 C:/Users/18504/Desktop/OS。基线 _lab2_B_2412224_20261023/baseline，待交付 _lab2_B_2412224_20261023/submission。预期仅 code/kern/mm/best_fit_pmm.c完整实现+学号，code/kern/mm/pmm.c一行管理器选择。请核验全扫描最小块、失败不改状态、切割链表顺序、前后合并、计数、框架两个测试完全不变、page_init2411259完全不变、tools/Makefile/entry.S/pmm.h完全不变。明确合法调用前提，勿把历史或模拟测试当真实make grade。指出实际缺陷和边界；返回逐项结论和行号。本次使用deepseek-official/deepseek-flash reasoning=max。不要联网或查看凭证。
````

这次审查实际成功使用 deepseek-official/deepseek-flash、reasoning=max；审查会话为 0395ee64-0519-4b7f-8978-eed5cb1a9476。审查本身只读，不代替运行测试。

## 4. 审查输出与人工复核

静态审查核对了全扫描最小适配、失败不改状态、余块切割、地址有序插入、前后合并和空闲页计数。主审采纳这些不变量检查，但当时没有采纳“0/20 是假阴性”“换版本即可通过”等未经当前副本实测的判断；后来 QEMU 4.1.1 的真实复跑补足了通过证据，旧批次仍如实记为失败。释放包含逐页清理与链表查找，复杂度为 O(n+k)，而非仅 O(k)。

本次迁移保留原有 Best-Fit 算法，填写 B 身份并改变管理器选择，没有已证实的 Best-Fit 算法错误需要编造修复记录。AI 曾把 file 输出 PGP Secret Sub-key 直接判断为镜像损坏，并据此猜测需要调整 objcopy 地址或启动文件；这些判断证据不足，后续用源码核对、启动输出及实际评分纠正。最终保留原启动链、page_init 和框架测试，在独立 QEMU 4.1.1 环境取得通过结果。

必须区分覆盖范围：原 best_fit_check 中 2 页加 1 页后申请 4 页失败属于总量不足；不能宣称独立验证了“总量充足但最大单块不足”。评分 Test 2 仅检查管理器指针，必须结合普通分配器自检，不能等同于完整边界测试。

## 5. 逐轮记录与本机留存

原始会话 seq 1–3520 中恢复了截至补齐请求 seq 3339 的 48 条用户消息，含 19 次图像引用、18 张不同附件。完整事件、图片、分工原文、审查全文、报告素材和运行日志已保存在仓库外的本机备份中。本文件直接收录实现任务、执行约束、审查提示和关键纠错原文，不以旧目录中的链接代替这些内容，也不声称此处包含全量聊天或全部图片。

分工中的预写提示属于建议模板，不等于每条都实际发送过；其中个人分支的建议后来被本人对 Lab2 分支和执行权限的要求取代。报告由小组负责报告的同学整合，本文件只汇总提示词和必要的协作说明。

## 6. 环境纠错链与验证结果

### seq 2952：限定修复范围

````text
请作为操作系统实验调试助手，解决当前 B 部分 Best-Fit 实验“编译成功但运行、评分失败”的问题。
1. 定位普通 QEMU 超时退出（124）及 `make grade` 退出 2、两项 WRONG、0/20 的根因，根据实际日志和源码修复。
2. 修改范围限于 Best-Fit 和管理器选择；不得修改队友的 `page_init()`、框架测试、启动配置或评分脚本。若修复必须超出范围，先说明原因并请求确认。
3. 修复后重新编译、自检和正式评分，以同次运行取得两个 PASS、20/20 为验收标准，保存真实运行证据。
4. 汇报根因、修改内容和实测结果；若仍失败，明确剩余阻塞，不得伪造成功。
全程不执行 commit 或 push。
````

### seq 3104：拒绝启动兼容性修改

````text
课程不允许上述启动兼容性改动，请改走课程认可的运行环境方案，保持原有修改边界，不修改评分脚本，也不通过包装器替换启动参数。
1. 先确认课程认可的环境要求；信息不足时向我询问，不自行假定。
2. 在认可环境中验证启动、自检及原版评分，如实报告结果；若仍有阻塞，提供证据，不擅自扩大修改范围。
仍不修改队友的 `page_init()` 和框架测试，不提交或推送。
````

### seq 3155：独立课程环境

````text
以课程实验指导书为准：
1. 按 QEMU 4.1.1 安装示例建立独立环境，不覆盖现有 QEMU；工具链发行包若仍无法确定，先向我确认。
2. 使用原启动参数验证启动，再运行普通自检和原版评分，报告结果。
3. 不修改启动配置、评分脚本、`page_init()` 或框架测试，不使用参数替换包装器，不执行 commit 或 push。若仍被启动代码阻挡，保留证据并停止，不越界修复。
````

### seq 3270：补充环境说明

````text
@C:\Users\18504\xwechat_files\wxid_gmxjuz1cbaju22_f8cb\msg\file\2026-09\3_startdash.md
````


最终使用独立 QEMU 4.1.1、OpenSBI v0.4 和经确认的 GCC 15.1.0：构建退出 0，普通启动出现 check_alloc_page() succeeded!；原版 make grade 两项 OK、Total Score: 20/20、退出 0，同次串口包含 [Test 1] Page Init: PASS 和 [Test 2] Best-Fit: PASS。普通 QEMU 的 15 秒观察超时退出 124 不代表内核自动退出，成功依据为实际自检输出。

历史 QEMU 7.0.0 的 0/20，以及一次将串口输出写到 Windows 挂载盘而报 grep: No data available 的 0/20，均保留为失败记录。后者改为 WSL /tmp 输出后复跑通过；未改写评分规则，也没有把后一次成功倒填进先前日志。完整证据继续留在本机，本文件不再要求通过旧目录链接读取。

## 7. 可复用的最终 spec（事后整理，非历史原文）

该版本仅方便小组复现或讨论，不能冒充第一次发送的 Prompt：

```text
在既有 Lab2 框架中实现 best_fit_init、best_fit_init_memmap、best_fit_alloc_pages、best_fit_free_pages、best_fit_nr_free_pages，并注册 best_fit_pmm_manager。遍历全部候选，选择长度至少为 n 的最小块，大小相同按地址序保留首候选；切割余块保持链表有序。释放先有序插入，再按物理邻接关系合并前后块。nr_free 只因真实分配、释放改变，分配失败不改状态。
仅修改 Best-Fit 实现与 init_pmm_manager 的选择语句；保留 page_init、框架测试、链表库、结构体、启动及评分配置。填写学号 2412224。
使用独立 QEMU4.1.1 与经确认的交叉工具链。执行 clean、build、原 make qemu、自检以及原 make grade；保留实际命令、退出码、完整日志、同次串口两个PASS和真实截图。不能用评分Test2指针检查替代普通best_fit_check。
发生启动层问题先核实环境，不越界改启动文件或评分规则；不commit/push。报告区分实测、静态推演和未覆盖场景。
```

---

## 任务三：buddy system（伙伴系统）分配算法

### 一：从零生成 Buddy System 分配模块并接入内核

[PROMPT]
**任务**：参考现有的 uCore 物理内存管理框架，新建 kern/mm/buddy_pmm.c 和 kern/mm/buddy_pmm.h 文件，从零实现 Buddy System 页面分配模块，并临时接入内核用于验证。

Buddy System 使用大小为 2^order 个页面的块。申请 n 页时，分配能够覆盖 n 的最小阶块；对应阶没有空闲块时从更高阶块逐级分裂。释放时只允许与同阶、空闲、且地址关系正确的真实伙伴块合并，并逐级向上合并。

**边界（务必遵守）**：
- 本任务只负责 Buddy 分配模块本身，不实现、不修改 page_init()。kern/mm/pmm.c 中 page_init() 当前只有调用、没有函数体（标注 //lab2 function1 YOUR CODE），这是另一个任务，即使编译/链接时出现 page_init 未定义，也不要自行补全它。
- struct Page 定义在共享的 kern/mm/memlayout.h 中，**绝对禁止修改**。Buddy 所需的块元数据只能复用现有字段。

**操作要求**：你必须在项目中修改实际文件，而不是仅展示代码。
保留现有代码，不删除既有逻辑，不修改与当前模块无关的文件。
允许修改/新增：
1. 新增 kern/mm/buddy_pmm.c；
2. 新增 kern/mm/buddy_pmm.h；
3. kern/mm/pmm.c（仅做最小接入：include 头文件 + 在 init_pmm_manager() 中临时切换）。

**输出要求**：直接进行文件操作，保留文件中的现有代码。在新增代码的注释中填写学号：2413551。

[RELY]

<relevant_structs>
Buddy 模块必须实现 struct pmm_manager（pmm.h）的 name 字段与 6 个函数成员：

```c
struct pmm_manager {
    const char *name;                              // "buddy_pmm_manager"
    void (*init)(void);
    void (*init_memmap)(struct Page *base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void (*free_pages)(struct Page *base, size_t n);
    size_t (*nr_free_pages)(void);
    void (*check)(void);
};
```

必须遵守框架的 Page 状态约定（与 First-Fit/Best-Fit 完全一致）：
1. PageReserved(p)：保留页（内核/描述符占用），分配器不可动。
2. PG_property 位 + property 字段：标记"空闲块头"及块内页数。**Buddy 中 property 存块大小（= 1 << order）**，分配时保留在块头 Page 中，供释放时反推 order。
3. set_page_ref(p, 0)：释放时清零引用计数。

<available_tools>
可直接使用以下现有符号与结构（不要重复定义）：

- pmm.h：pages、npage、nbase、page2ppn()、page2pa()、pa2page()、PPN()、page_ref()、set_page_ref()、alloc_pages/free_pages/nr_free_pages 等。
- memlayout.h：struct Page、free_area_t（含 list_entry_t free_list 与 unsigned int nr_free）、PageReserved/PageProperty/SetPageProperty/ClearPageProperty 等宏。
- list.h：list_init、list_add、list_add_before、list_add_after、list_del、list_empty、list_next、list_prev、le2page 等双向链表操作。
- defs.h：ROUNDUP、ROUNDDOWN。
- 全局可管理页数上限：npage 表示物理页号上界，伙伴块越界判断用 page2ppn() 与 npage 比较。
</available_tools>

<relevant_files>
1. kern/mm/default_pmm.c —— 参考管理器接入方式、Page 状态约定、init_memmap 的预处理与 check 的组织方式。
2. kern/mm/pmm.c —— init_pmm_manager() 与统一分配接口。
3. Makefile —— 无需修改；kern/mm/ 下的新 .c 会被自动编译。
</relevant_files>

[GUARANTEE]
至少实现并注册：
1. buddy_init(void)
2. buddy_init_memmap(struct Page *base, size_t n)
3. buddy_alloc_pages(size_t n)
4. buddy_free_pages(struct Page *base, size_t n)
5. buddy_nr_free_pages(void)
6. buddy_check(void)（本任务只需最小自检，见 SPECIFICATION 第 8 节）
7. const struct pmm_manager buddy_pmm_manager
8. buddy_pmm.h 中的外部声明

允许自行添加 static 辅助函数（需说明每个函数维护的不变量），例如：
- 计算覆盖 n 的最小 order；
- 验证指定 order 的块对齐；
- 将初始内存区分解为合法的 2 的幂块；
- 在指定 order 的空闲结构中插入/移除块；
- 计算伙伴块；
- 分裂高阶块；
- 合并同阶伙伴。

[SPECIFICATION]

## 1. order 与块
- order=k 的块包含 2^k 个连续物理页。
- 每个空闲块必须满足伙伴对齐：其起始页号 page2ppn(block) 的低 k 位必须全为 0，即 (page2ppn(block) & ((1UL<<k)-1)) == 0。
- 伙伴块页号 = page2ppn(block) ^ (1UL << k)。
- 最大 order 用静态常量 MAX_ORDER（建议取 20），必须覆盖 128MB（= 2^15 页），且保证 1UL << MAX_ORDER 不发生移位溢出（64 位下 MAX_ORDER < 64）。
- 同一页面不能同时属于两个空闲块。

## 2. 初始化（buddy_init）
- 所有 order 的空闲结构均初始化（每个 order 的 free_list 用 list_init，nr_free 置 0）。
- 总空闲页计数置 0，不残留旧链表节点与统计值。

## 3. 注册初始空闲区（buddy_init_memmap）
前置条件：base 非空；n > 0；[base, base+n) 是 page_init() 交给分配器的连续可用范围，可能不是 2 的幂大小，也可能不满足最大块对齐。
预处理（与 default_init_memmap 对齐）：对 [base, base+n) 每页 assert(PageReserved(p))，然后清 flags、set_page_ref(p, 0)。
后置效果：
- 将整个范围贪心分解为若干互不重叠、各自满足对齐的 2 的幂块；所有块容量之和恰好为 n，不丢头尾页，不把范围外页面加入。
- 只有块首 Page 保存有效元数据（property = 1<<order，置 PG_property）。
- nr_free 恰好增加 n。

## 4. 分配（buddy_alloc_pages）
前置条件：n > 0。
失败情况：不存在足够容量的块时返回 NULL，且不得改变任何空闲结构、Page 状态或 nr_free。
成功情况：
- 计算覆盖 n 的最小 order；优先从对应 order 取；该阶为空则从更高阶逐级二分，每次分裂出的另一半（buddy）挂入正确的下一阶空闲结构。
- 返回已分配块的首个 Page，并在块头保留 property = 1<<order（供释放反推 order）。
- nr_free 按实际占用块容量（1<<order）减少；返回范围不与其它已分配块重叠。
- 非 2 的幂请求（如 3、5 页）按向上取整处理（3 页占用 4 页），统计与释放均按实际块容量。

## 5. 释放（buddy_free_pages）
前置条件：base 是此前成功分配块的首地址；实现用 base->property 反推实际 order。
约束：用 assert 约束非法释放（与 First-Fit 一致）——释放的页应满足 !PageReserved 且 !PageProperty；base 的 property 应为 2 的幂且块对齐，否则 assert 失败。
释放效果：
- 恢复该块空闲状态（SetPageProperty，property=1<<order）。
- 计算同阶伙伴 page2ppn(base) ^ (1<<order)；仅当伙伴位于管理范围（page2ppn < npage）、当前空闲（PageProperty 为真）、order 相同、且地址关系正确时才合并。
- 每次合并产生更高一阶块，重复直到伙伴不可合并或达到 MAX_ORDER；非伙伴块即使物理相邻也不合并。
- nr_free 只增加本次释放块的容量，合并过程不重复增加 nr_free。

## 6. 空闲页统计
- buddy_nr_free_pages() 返回实际空闲页总数，必须等于所有 order 空闲块容量之和。
- 分裂与合并不改变空闲页总数；分配与释放按实际块容量增减计数。

## 7. 接入要求
- buddy_pmm_manager 的函数指针必须完整，name = "buddy_pmm_manager"。
- 在 kern/mm/pmm.c 顶部 #include <buddy_pmm.h>，并在 init_pmm_manager() 中把 pmm_manager 临时切换为 &buddy_pmm_manager（保留 default/best_fit 的原有 include 与代码不变）。
- pmm_manager->check() 应调用 buddy_check()；项目中其他分配器（default、best_fit）仍能正常编译。

## 8. 最小自检（buddy_check，本任务范围）
本任务只需实现最小自检，用于确认模块基本可用：校验 nr_free 等于遍历所有 order 空闲块容量之和；做一次简单的单页/多页分配与释放往返，确认计数守恒。
系统化的边界与压力测试属于后续独立任务，本任务不要实现。




### 二：针对第一份提示词生成的分配算法设计相应的测试函数

[PROMPT]
**任务**：针对已实现的 Buddy System 页面分配模块（kern/mm/buddy_pmm.c），设计并实现完整的测试函数，以检验不同大小内存对象的分配和释放是否正确。

测试的重点是 Buddy 分配器的核心行为与边界情况：不同阶（不同大小）块的分配、非 2 的幂请求的向上取整、高阶块的分裂、释放时的伙伴合并、空闲页计数守恒，以及越界/非法请求的处理。

测试通过 buddy_check() 入口执行：pmm_manager->check() 会调用 buddy_check()，从而在 make qemu 时被 check_alloc_page() 自动执行。

**边界（务必遵守）**：
- 只允许修改 kern/mm/buddy_pmm.c 中的测试相关部分（buddy_check 及其 static 辅助测试函数），不得改动 Buddy 分配算法的实现逻辑。
- 测试必须使用框架公开的分配/释放接口（alloc_pages/free_pages/alloc_page/free_page），不要直接操纵 Buddy 内部数据结构，除非是为了校验不变量。
- 测试中通过 assert 断言正确性；测试结束后必须将空闲链表和 nr_free 恢复到测试前的状态，不得污染分配器状态（参考 default_check 的做法：保存 free_list/nr_free 快照，结束时恢复）。

**操作要求**：你必须在项目中修改实际文件，而不是仅展示代码。
保留现有代码，不删除既有逻辑。允许修改：
1. kern/mm/buddy_pmm.c（仅扩展 buddy_check 及其 static 测试辅助函数）。

**输出要求**：直接进行文件操作，保留文件中的现有代码。在新增测试代码的注释中填写学号：2413551。

[RELY]

<relevant_structs>
Buddy 分配模块的接口契约（测试必须依据这些契约）：
- buddy_alloc_pages(size_t n)：申请 n 页，返回覆盖 n 的最小 2 的幂块的首 Page；失败返回 NULL 且不改变任何状态。
- buddy_free_pages(struct Page *base, size_t n)：释放块首为 base、容量为 n（2 的幂）的块；与同阶、空闲、地址正确的伙伴逐级向上合并。
- buddy_nr_free_pages(void)：返回空闲页总数。
- 块元数据约定：块首 Page 的 property = 1<<order，PG_property 位标记空闲块头；分配后保留 property 供释放反推 order。
</relevant_structs>

<available_tools>
可直接使用的现有符号与接口（不要重复定义）：
- pmm.h：alloc_pages()、free_pages()、alloc_page()、free_page()、nr_free_pages()、page2ppn()、page2pa()、page_ref()、pages、npage、nbase。
- memlayout.h：PageProperty()、SetPageProperty()、ClearPageProperty()、PageReserved()、free_area_t、le2page。
- list.h：list_init、list_empty、list_next、list_prev。
- 标准库：cprintf 用于输出测试通过/失败信息（与 best_fit_check 中 #ifdef ucore_test 的分级输出风格保持一致，可选）。
</available_tools>

<relevant_files>
1. kern/mm/buddy_pmm.c —— 被测模块及其 buddy_check 入口，本任务在此扩展测试。
2. kern/mm/default_pmm.c 的 default_check、kern/mm/best_fit_pmm.c 的 best_fit_check —— 参考测试的组织方式（保存/恢复 free_list 与 nr_free 快照、用 assert 校验、按场景逐步验证）。
</relevant_files>

[GUARANTEE]
buddy_check() 至少覆盖以下测试场景（可拆分为多个 static 辅助函数被 buddy_check 依次调用）：
1. 分配正确性：不同大小（1、2、3、4、5、8、16 页）分配返回非 NULL、块首地址满足对应 order 对齐。
2. 向上取整：申请 3 页实际占用 4 页、申请 5 页实际占用 8 页（通过分配前后 nr_free 差值验证）。
3. 分裂正确性：分配小阶块触发高阶块逐级分裂后，剩余伙伴被正确挂回空闲链表。
4. 释放与合并：释放后与伙伴正确合并为更高阶块（通过 nr_free 恢复与空闲块容量之和验证）。
5. 计数守恒：一系列随机/混合分配与释放后，nr_free 等于所有 order 空闲块容量之和，且与初始空闲页数一致。
6. 失败路径：申请超过总空闲页数时返回 NULL，且 nr_free 与空闲链表不被改变。
7. 非法释放：对未分配/非块首/重复释放的页面触发 assert（可选，用注释说明预期行为）。

测试结束后必须恢复到测试前状态，保证不影响后续执行。

[SPECIFICATION]

## 1. 测试入口与恢复
- buddy_check() 是 pmm_manager->check 的入口，被 check_alloc_page() 调用。
- 测试开始时保存 free_list 与 nr_free 的快照；测试过程中所有临时改动在结束前恢复。
- 每个场景通过 assert 校验，失败即触发 panic，便于定位。

## 2. 不同大小对象的分配
- 依次申请 1、2、3、4、5、8、16 页，断言均返回非 NULL，且返回的 Page 是块首（PageProperty 已清除，property 为 2 的幂）。
- 验证 3 页请求实际占用 4 页：alloc 前后 nr_free 的差值应等于 4（而非 3）；5 页请求差值应等于 8。
- 验证块对齐：申请 order=k 的块，其 page2ppn() 的低 k 位必须为 0。

## 3. 分裂路径
- 构造场景：先分配一个高 order 块，再分配一个低 order 块，强制触发分裂。
- 断言分裂产生的另一半伙伴（空闲、同阶、地址为原块 ^ 偏移）进入了正确的空闲链表，且 nr_free 守恒。

## 4. 释放与伙伴合并
- 分配两个相邻的 2^k 页块（构成一对伙伴），按正确顺序释放，断言合并为 2^(k+1) 的块（nr_free 增加量与合并结果相符）。
- 释放非伙伴块（物理相邻但地址关系不正确）时，断言不会发生合并（nr_free 只增加该块容量）。

## 5. 计数守恒
- 设计一次混合操作序列（多次分配不同大小 + 部分释放 + 再分配），每一步后都断言 nr_free == Σ(各 order 空闲块容量)。
- 序列结束、恢复快照后，断言 nr_free 与初始值一致。

## 6. 失败与边界
- 记录当前总空闲页数 N，申请 N+1 页（向上取整后的块容量超过 N）断言返回 NULL，且 nr_free、空闲链表均未改变。
- 分配 1 页后再释放，往返后 nr_free 不变。

## 7. 输出
- 测试通过时用 cprintf 输出成功信息（如 "[Buddy] all checks passed"）；参考 best_fit_check 可选用 #ifdef ucore_test 分级输出，非必需。

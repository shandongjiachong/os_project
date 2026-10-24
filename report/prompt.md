# Lab2 提示词汇总

本文件汇总王泊凯（2412224）的 Best-Fit 提示记录与李睿（2413551） 的 Buddy System 两份提示词。

# B 部分实际 AI 协作记录

姓名：王泊凯；学号：2412224。以下区分真实发送记录、预写分工提示词和事后说明，不把它们混为同一回事。

## 1. 首次实际用户 Prompt

[完整原文](<B-best-fit/evidence/initial-user-prompt.txt>)从本会话事件 seq 9 提取，要求读取分工、代码和指导书，并使用 DeepSeek v4.1 Flash max。原分工中的预写提示不是每一轮实际对话的替代品。

## 2. 最初实现任务及实际执行者

[Best-Fit 任务提示](<B-best-fit/evidence/historical-best-fit-dispatch.txt>)来自实际工具调用 seq 87，包含全部接口、全扫描最小适配、切割、合并和禁止修改测试的要求。文件保留历史代码字符串中的转义反引号。

该次请求的路由是 deepseek/deepseek-v3-250324，返回错误：


> child LLM route "deepseek/deepseek-v3-250324" is not allowed for this Session

这既不是用户指定的 Flash 路由，也没有成功执行。不能写成“DeepSeek Flash 首次生成并通过验证”。历史源码由主助手在后续工具调用（seq 108；管理器选择 seq 122）写入。本次没有重新虚构一次算法生成过程。

## 3. 后续实际约束与最终工作 Prompt

- [身份与分支要求原文](<B-best-fit/evidence/identity-and-branch-prompt.txt>)：明确王泊凯、2412224、无需个人分支。其当时的“只给命令”约束后来由下一条执行要求取代。
- [最终执行 Prompt 完整原文](<B-best-fit/evidence/current-user-request.txt>)：只做 B；隔离目录；保留队友与测试；真实编译、普通运行和正式评分；保存真实截图；不得 commit/push；失败必须如实记录。本次按这条执行。
- [只读审查 Prompt 完整原文](<B-best-fit/evidence/review-prompt.txt>)：发送给实际成功运行的 deepseek-official/deepseek-flash、reasoning=max 子智能体，要求检查最小适配、顺序、计数、合并和保护范围，不构建、不改文件。会话 ID：0395ee64-0519-4b7f-8978-eed5cb1a9476。

## 4. 审查输出与人工复核

[原始审查结果](<B-best-fit/review/deepseek-flash-max-raw.md>)和[主审复核](<B-best-fit/review/主审复核.md>)同时保留。采纳静态不变量检查；当时不采纳“0/20 是假阴性”“换版本即可通过”等未经本副本运行验证的判断；后来 QEMU 4.1.1 的真实复跑补足通过证据，旧批次仍如实记为失败。主审还据实际逐页清理循环，把释放总复杂度明确为 O(n+k)，而非仅记链表搜索的 O(k)。

本次代码迁移保留原有 Best-Fit 算法，只填写 B 身份并在远程基线上改变管理器选择。没有已证实的 Best-Fit 算法错误需要编造修复记录。历史有关 PGP 文件类型、objcopy 偏移及镜像大小的错误判断已在[实现说明](<B-best-fit/说明.md>)明确纠正，未混入本次源码。

## 5. 已恢复的逐轮原文与图片反馈

本轮通过原始事件读取接口，连续扫描会话 seq 1–3520，并按 source.kind=user 筛选，恢复截至本次补齐请求 seq 3339 的 48 条真实用户消息。包含图像反馈 19 次引用，对应 18 张不同附件；每张按 SHA-256 原样保存，不改图、不把截图 OCR 结果冒充用户文本。完整文本与图像链接见[逐轮原文](<B-best-fit/evidence/prompt-history/README.md>)，机器可读数据见[events.json](<B-best-fit/evidence/prompt-history/events.json>)。该范围的用户消息已完整恢复；这不是助手全部回答或所有工具调用的全量聊天备份。

原分工中的预写消息保存在[分工原文](<B-best-fit/evidence/B-assignment-original.txt>)。这些是建议发送的模板，不等于每一条都实际发送过。原文中的个人分支建议后来被本人明确要求的 Lab2 目标分支及禁止本轮 commit/push 所取代。

## 6. 环境纠错链与本轮收敛

- seq 2952：将修复范围限定为 Best-Fit 与管理器选择，禁止更改启动链和测试。
- seq 3104：明确拒绝启动兼容性修改，要求改走课程认可环境。
- seq 3155：同意独立安装 QEMU 4.1.1；编译器版本若不确定先确认。
- 工具交互确认：本人选择“允许沿用现有工具链”，故保留 GCC 15.1.0；不将它标注为 SiFive 官方发行。
- seq 3270：补充课程本地环境说明；该文没有指定工具链发行版本。
- seq 3339：要求补齐 B 的全部材料。

普通自检与原版评分已在 QEMU 4.1.1 下实测通过，原失败记录作为历史保留。此前“不保证换版本即可通过”是当时正确的证据边界，后来通过真实运行补足了结论，并非事后删除失败过程。详见[实现报告](<B-best-fit/说明.md>)与[验证证据](<B-best-fit/evidence/README.md>)。

## 7. 可复用的最终 spec（事后整理，非历史原文）

该版本仅方便小组复现或讨论，不能冒充第一次发送的 Prompt：

```text
在既有 Lab2 框架中实现 best_fit_init、best_fit_init_memmap、best_fit_alloc_pages、best_fit_free_pages、best_fit_nr_free_pages，并注册 best_fit_pmm_manager。遍历全部候选，选择长度至少为 n 的最小块，大小相同按地址序保留首候选；切割余块保持链表有序。释放先有序插入，再按物理邻接关系合并前后块。nr_free 只因真实分配、释放改变，分配失败不改状态。
仅修改 Best-Fit 实现与 init_pmm_manager 的选择语句；保留 page_init、框架测试、链表库、结构体、启动及评分配置。填写学号 2412224。
使用独立 QEMU4.1.1 与经确认的交叉工具链。执行 clean、build、原 make qemu、自检以及原 make grade；保留实际命令、退出码、完整日志、同次串口两个PASS和真实截图。不能用评分Test2指针检查替代普通best_fit_check。
发生启动层问题先核实环境，不越界改启动文件或评分规则；不commit/push。报告区分实测、静态推演和未覆盖场景。
```

---

# 功能模块二

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

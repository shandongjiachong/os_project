# 子智能体原始审查意见（未经采信的原文）

> 本文件保留原始意见，不代表主审结论。关于“假阴性”“换版本即可通过”和未核实文件的说法须以主审复核为准。

以下为只读审查结果。本次审查未修改任何文件，未构建、未运行 QEMU、未联网、未查看凭证、未 commit/push；运行日志来自此前外部验证进程写入 `evidence` 的记录，我只读取。

## 结论摘要

1. **代码静态实现符合 B Best-Fit 作业要求**，未发现阻断性代码缺陷：全扫描最小块、失败不改状态、切割保序、前后合并、计数都正确。
2. **没有取得有效的 `make grade` 结果**。`evidence` 里的 0/20 是 QEMU 7.0 启动方式导致的假阴性，不是 Best-Fit 断言失败；作业要求的 QEMU 4.1.1 下尚未复跑，因此不能声称“已通过”。
3. 报告仍是空白模板；若课程要求提交报告，这是待补项。代码副本本身只有两个文件发生变更。

---

## 一、交付差异与不变项

### 1. 变更范围

`diff -rq baseline submission` 的代码差异只有两处：

| 文件 | 差异 | 结论 |
|---|---|---|
| `code/kern/mm/best_fit_pmm.c` | 138 行 → 279 行；新增实现位于 8–137 行，管理器结构位于 271–279 行 | 符合“完整实现 + 学号” |
| `code/kern/mm/pmm.c` | 150 行；仅第 37 行改成 `pmm_manager = &best_fit_pmm_manager;` | 符合“一行管理器选择” |
| `submission/report/B-best-fit/` | 新增 evidence 目录 | 审查证据，不是课程代码 |

包内没有 `.git` 目录；没有 commit/push 痕迹。

### 2. 框架两个测试完全不变

做了逐字节比较：

```text
baseline best_fit_pmm.c 第 8–138 行
submission best_fit_pmm.c 第 139–269 行
diff 结果为空
```

即：

- `basic_check`：baseline 9–58 行 ↔ submission 140–189 行；
- `best_fit_check`：baseline 60–138 行 ↔ submission 191–269 行；
- submission 139 行的框架注释也完全一致。

`best_fit_check` 中的关键断言，例如 201 行 `total == nr_free_pages()`、252 行 `nr_free == 0`、263–264 行 `count == 0 && total == 0`，都没有被改动。

### 3. page_init（含 2411259 注释）完全不变

`pmm.c`：

- 第 64 行：`//lab2 function1 2411259`；
- 第 65–123 行：`page_init` 本体；
- 与 baseline 的 64–123 行逐字节相同（`diff` 为空）。

注意：`2411259` 是模板里原有的学号注释；作业要求 `page_init` 完全不变，所以**保持原样是正确的**，不要改成 2412224。

`pmm_init` 126–145 行、`check_alloc_page` 147–150 行也未变；唯一改动就是 37 行。

### 4. tools / Makefile / entry.S / pmm.h 完全不变

关键文件 SHA-256 在 baseline 与 submission 下相同：

| 文件 | SHA-256 |
|---|---|
| `code/kern/mm/pmm.h` | `afd175be22d87008e4070dde7fc3c1cebe876b55aafeb6fb9a335f8fd66c93bc` |
| `code/Makefile` | `7e08d809582b5600eef924d4eb5d2f405b8cfd168a40804b32961aea6871e89f` |
| `code/kern/init/entry.S` | `0f32d455dd8dcaa99987eb2eafd64df5e7535e14477ed2aece8e599e3b911111` |
| `tools/test_lab2.c` | `6feb75f20361d95436856e525af80bef0c55b300391cda3a6deaeda53a0f118d` |
| `tools/test_lab2.h` | `9caead317ad13c5cdd143abbc3f250c9b8f284125c4ba17a1e4f479b8d9f444c` |
| `tools/grade.sh` | `9f4b6a6277daa092b70706a67a656dc3978acb0eb8139352a3a7b21ec9f522b1` |

另外 `best_fit_pmm.h`、`default_pmm.c`、`trap.c`、`tools/inject.py` 等也全部相同；由于 `diff -rq` 只有两处代码差异，其余文件无需逐个担心。

`Makefile` 的 QEMU 命令（163–168 行）和 grade 目标（205–212 行）未改；`grade.sh` 236–241 行的两个 PASS 匹配逻辑未改；`test_lab2.c` 100/110 行的 PASS 输出未改。

---

## 二、Best-Fit 实现逐项核验（行号）

### 1. 全扫描最小块

`best_fit_pmm.c` 48–79 行，扫描在 54–64 行：

- 56–57 行：从 `free_list` 头开始遍历；
- 59 行：只考虑 `p->property >= n`；
- 60 行：`best == NULL || p->property < best->property` 时更新；
- 没有“找到第一个就 break”，确实扫描全部空闲块；
- 严格小于才更新，所以大小相同时保留链表中的第一个（地址最低）块。Best-Fit 对平局没有唯一要求，这个选择合法且稳定。

结论：**全扫描最小块，正确**。

### 2. 失败不改状态

- 50 行：`assert(n > 0)`；`n == 0` 是调用者违约，会直接断言，不是“返回 NULL”。
- 51–53 行：`n > nr_free` 时立即返回 NULL，未动链表、未动计数。
- 54–65 行：扫描后 `best == NULL`（即 `nr_free` 够但碎片化、没有单块 >= n）时，65 行分支不进入，直接返回 NULL，同样不改状态。
- 只有进入 65 行分支后才会 `list_del`、切块、减 `nr_free`；不存在“改了一半再失败”的路径。

结论：**合法失败路径（容量不足/无合适块）不改状态，正确**。

### 3. 切割与链表顺序

65–77 行：

- 66 行先保存 `prev = list_prev(&best->page_link)`；
- 67 行 `list_del(best)`；
- 68 行仅当 `best->property > n` 时切割；
- 69–71 行：`p = best + n`，`p->property = best->property - n`，`SetPageProperty(p)`；
- 72 行：`list_add(prev, &p->page_link)`，即插回 `best` 原来的位置（`prev` 之后）；
- 74–75 行：`nr_free -= n`，`ClearPageProperty(best)`。

因为空闲链表按地址升序，`best` 的后继块地址必然大于 `best + best->property`，也就大于 `best + n`；插到 `prev` 之后不会破坏升序。`best` 是头块时 `prev` 是哨兵，`list_add(&free_list, ...)` 插到头部；`best` 是尾块时插到尾部，也都正确。

结论：**切割余块属性、标志、插入位置正确，链表保持升序**。

### 4. 前后合并

`free_pages` 81–132 行：

- 84–89 行：逐页 `assert(!PageReserved(p) && !PageProperty(p))`，清 `flags`、`ref`；
- 90–92 行：`base->property = n`、`SetPageProperty(base)`、`nr_free += n`；
- 94–97 行：空链表直接 `list_add` 后返回；
- 99–109 行：按地址找插入点，`list_add(insert_pos, ...)` 保持升序；
- 111–120 行：先与**后块**合并：
  - 115 行 `base + base->property == p` 精确相邻；
  - 116 行 `base->property += p->property`；
  - 117 行 `ClearPageProperty(p)`；
  - 118 行 `list_del(p)`；
- 122–131 行：再与**前块**合并：
  - 126 行 `p + p->property == base` 精确相邻；
  - 127 行 `p->property += base->property`；
  - 128 行 `ClearPageProperty(base)`；
  - 129 行 `list_del(base)`。

先合并后块、再合并前块是正确顺序：后块并入 `base` 后，`base` 的 property 已覆盖后块；前块判断仍用 `base` 的起始地址，不受影响。前后都相邻时，最终由前块吸收整个区间。合并过程不再改 `nr_free`，因为 92 行已经加过一次，正确。

结论：**前后合并、被并块清标志并摘链、计数不重复，正确**。

### 5. 计数

- `best_fit_init` 13–17 行：`nr_free = 0`；
- `best_fit_init_memmap` 30 行：`nr_free += n`；
- `best_fit_alloc_pages` 74 行：仅成功分配时 `nr_free -= n`；
- `best_fit_free_pages` 92 行：仅释放时 `nr_free += n`；
- `best_fit_nr_free_pages` 134–136 行：返回 `nr_free`。

`best_fit_check` 201 行验证 `total == nr_free_pages()`，252 行验证 `nr_free == 0`，263–264 行验证遍历计数归零。

结论：**计数路径完整，未发现漏加/漏减**。

### 6. 管理器结构与选择

`best_fit_pmm.c` 271–279 行：

```c
const struct pmm_manager best_fit_pmm_manager = {
    .name = "best_fit_pmm_manager",
    .init = best_fit_init,
    .init_memmap = best_fit_init_memmap,
    .alloc_pages = best_fit_alloc_pages,
    .free_pages = best_fit_free_pages,
    .nr_free_pages = best_fit_nr_free_pages,
    .check = best_fit_check,
};
```

名称与 `best_fit_pmm.h` 的 `extern` 声明一致；`pmm.c` 第 37 行选择它；`pmm_init` 132 行 init、136 行 page_init、139 行 check_alloc_page，顺序正确。

结论：**管理器定义与选择正确**。

### 7. 框架测试的覆盖含义

`best_fit_check` 235–237 行是真正区分 Best-Fit 与 First-Fit 的地方：

- 此时空闲链表为 `[p0+1, property=2]`、`[p0+4, property=1]`；
- `alloc_pages(1)` 必须返回 `p0+4`（最小合适块）；
- First-Fit 会返回 `p0+1`，237 行断言失败。

而注入的 `test_lab2.c` 的 `test_best_fit`（103–111 行）只检查 `pmm_manager == &best_fit_pmm_manager`，110 行打印 Test 2 PASS，**并不检验分配策略**。所以“Test 2 PASS”本身不能证明 Best-Fit 正确；真正兜底的是 `check_alloc_page()` 调用的 `best_fit_check`（`pmm.c` 147–150 行）。这一点在解读 grade 结果时要注意。

---

## 三、合法调用前提

代码实现正确，但它的正确性依赖以下调用契约；这些也是“边界”所在：

### 1. 初始化与内存注册

- `best_fit_init()` 必须先执行一次，且只执行一次；它清空链表、置 `nr_free=0`。
- `best_fit_init_memmap(base, n)` 要求：
  - `n > 0`（21 行断言）；
  - `[base, base+n)` 内每一页当前都是 `PageReserved`（24 行断言），这与 `page_init` 先标 reserved 再注册空闲页的流程一致；
  - 该区间不能与已在空闲链表中的区间重叠；
  - 本实验 `page_init` 只在 136 行调用一次（`pmm.c` 内），所以安全。
- 若多次调用 `init_memmap`：插入逻辑仍能维持地址升序，但**不会把相邻区间合并**；调用者要么保证多次注册的区间互不相邻，要么只调用一次。本实验只调用一次，不触发。

### 2. 分配

- `alloc_pages(n)` 要求 `n > 0`（50 行断言）。
- 调用者必须保证 `nr_free` 与空闲链表实际总页数一致；否则 51 行的提前返回可能误判。
- 单 hart、无并发；该实现没有锁，也不能在中断/多线程环境重入。
- 返回的是一段连续的 `n` 页；调用者不得越界使用，也不得释放未分配的页。

### 3. 释放

- `free_pages(base, n)` 要求 `n > 0`（83 行断言）。
- `[base, base+n)` 内所有页必须当前已分配、非 reserved、且不在空闲链表中（86 行只逐页检查 `!PageReserved && !PageProperty`）。
- **允许释放一个大块中的子区间**：框架测试确实这样用（`best_fit_check` 230–231 行释放 `p0+1,2` 和 `p0+4,1`；244、256 行释放整个 `p0,5`，其中包含多次分配的子块）。因此实现不能假设“一次释放必须对应一次分配”。
- 但调用者必须保证区间内没有已经空闲的页、没有重复释放、没有与既有空闲块重叠；这些条件 86 行的逐页断言抓不住，违反了会破坏链表。
- 释放后到再次分配前，不要并发访问这些页。

### 4. 选择与检查

- 必须在 `pmm.c` 37 行选择 `best_fit_pmm_manager`；
- `pmm_init` 必须先 init、再 page_init、再 check_alloc_page；当前 126–145 行顺序正确；
- `best_fit_check` 是框架测试，不能修改；它通过 `check_alloc_page` 在启动时执行。

---

## 四、实际缺陷与边界

### 阻断性缺陷

**未发现。** 就本实验的单次 `page_init`、单 hart、框架给定的 `basic_check`/`best_fit_check` 用法而言，当前实现可以通过静态推演；编译也在外部证据中通过。

### 潜在缺陷/边界（不触发本实验）

1. **多次 `init_memmap` 不合并相邻区间**（31–45 行）。  
   本实验只调用一次，所以不是本次缺陷；若将来框架多次注册相邻物理区间，空闲链表会出现相邻节点，破坏“空闲块极大化”的不变量，后续合并逻辑可能留下相邻块。严重度：低（本实验不触发）。

2. **`free_pages` 没有所有权/重叠/重复释放检测**（84–92 行）。  
   86 行只检查每页的 `PageReserved`/`PageProperty`。重复释放一个空闲块的非块头页时，该页 `PageProperty` 为 0、非 reserved，断言会通过，随后把它当成新块头插入，形成重叠节点，链表被破坏。  
   这是 ucore `default_pmm` 也存在的同类边界，不是本实现独有的错误；但它是“合法调用前提”的一部分。严重度：中（调用者违约时）。

3. **已分配块头/被并块保留旧 `property` 值**（75、117、128 行只清标志）。  
   `PageProperty` 是判断块头的权威标志，旧 property 不会被测试或分配逻辑误用；这是标准 ucore 写法。严重度：无（仅可读性）。

4. **`nr_free` 是 `unsigned int`**（`free_area_t`，`memlayout.h` 57–60 行）。  
   本实验 128MiB、32768 页，远小于上限；若物理内存极大（超过 2^32 页）才会溢出。严重度：无（本实验）。

5. **无锁、不可重入**。  
   单 hart、启动期初始化，本实验不需要锁；若将来在中断/多线程中使用，需要外层保护。严重度：低（本实验不触发）。

6. **学号注释包含姓名**（6 行：`//lab2 function2-2 2412224 王泊凯`）。  
   作业只要求把 `YOUR CODE` 替换为学号；多写姓名属于可接受超集，不影响编译/测试。若老师严格要求只写学号，可删姓名，但当前不算错误。

7. **`Test 2` 只验管理器指针**（`test_lab2.c` 103–111 行）。  
   这不是提交代码的缺陷，而是评分覆盖面的边界：真正的 Best-Fit 策略由 `best_fit_check` 兜底；不要因为“Test 2 PASS”就认为策略已被验证。

### 非代码交付项

- `submission/report/prompt.md` 为 0 字节；`submission/report/实验报告模板.md` 5518 字节，哈希与 baseline 相同，**报告尚未填写**。若课程要求报告，这是待补项。
- `submission/report/B-best-fit/evidence` 是额外的本地产物（构建/验证日志），不是课程代码；提交前按课程要求决定是否保留。

---

## 五、真实 `make grade` 证据与结论

### 1. 编译是通过的

`evidence/build.exitcode = 0`，`build.log` 显示：

- `riscv64-unknown-elf-gcc 15.1.0` 编译全部源文件；
- 包括 `best_fit_pmm.c`、`pmm.c`、`default_pmm.c`；
- `ld bin/kernel`、`objcopy` 成功；
- `bin/kernel` 96840 字节，`bin/ucore.img` 24600 字节；
- ELF 入口 `0xffffffffc0200000`，两个 LOAD 段 VMA/PhysAddr 均为 `0xffffffffc0200000` 起。

Makefile 的 `-Wall -Werror` 下能编译通过，说明没有编译警告/错误。

### 2. `make qemu` / `make grade` 的 0/20 是启动失败假阴性

`evidence` 中：

- `qemu.exitcode = 124`（15 秒超时）；
- `grade.exitcode = 2`；
- `grade.log`：`Total Score: 0/20`，missing `[Test 1] Page Init: PASS`、missing `[Test 2] Best-Fit: PASS`；
- `qemu.log` / `grade-serial.log`：**只有 OpenSBI v1.0 横幅**，`Domain0 Next Address : 0x0000000000000000`，没有哪怕一行内核输出（连 `memory management:` 都没有）。

这说明内核根本没有开始执行，而不是 Best-Fit 断言失败。0/20 不能作为代码不合格的证据。

### 3. 原因：QEMU 7.0 的启动方式与作业 QEMU 4.1.1 不同

- 环境日志：`QEMU emulator version 7.0.0`，路径 `/opt/qemu/bin/qemu-system-riscv64`；
- `Makefile` 163–168 行使用 `-device loader,file=$(UCOREIMG),addr=0x80200000`，没有 `-kernel`；
- QEMU 7.0 默认 OpenSBI v1.0 固件走 fw_dynamic 路径，没有 `-kernel` 时 next stage 被设为 0，于是 OpenSBI 跳到 0，内核未执行；
- 作业原文要求 QEMU 4.1.1，其默认 fw_jump 固定跳到 `0x80200000`，同一 Makefile 命令才能工作。

历史日志 `_tmp_code/lab2_verify_a6qqzevw/grading.qemu.log`、`best-fit.qemu.log`（2026-10-23，旧源码树）显示：

- `Domain0 Next Address : 0x0000000080200000`；
- 内核正常打印，`check_alloc_page() succeeded!`；
- 两个 PASS 都出现。

但那是**旧树 + 另一种启动方式**：`_tmp_code/lab2_qemu_verify.py:26` 用的是 `-bios default -kernel image`，不是本副本的 `make grade`。因此它只能佐证“本次失败是启动方式差异”，**不能当成本次 submission 的真实 grade 证据**。

### 4. 有效结论

- **有效 `make grade`：尚未取得。**
- 当前 0/20：环境假阴性，不代表 Best-Fit 失败。
- 交付前必须在课程要求的 QEMU 4.1.1 环境重新执行 `make clean && make && make qemu` / `make grade`，确认：
  - `check_alloc_page() succeeded!`
  - `grading: 6 / 6 points`
  - `[Test 1] Page Init: PASS`
  - `[Test 2] Best-Fit: PASS`
- 在这些出现之前，不能对外声称“已通过 make grade”。

---

## 六、逐项结论表

| 检查项 | 结论 | 依据 |
|---|---|---|
| 仅两个代码文件变更 | 符合 | `diff -rq`；`best_fit_pmm.c` + `pmm.c` |
| 学号写入 | 符合 | `best_fit_pmm.c` 6 行 |
| 全扫描最小块 | 通过（静态） | 54–64 行，60 行取最小 |
| 失败不改状态 | 通过（静态） | 50–53 行、65 行分支 |
| 切割与链表升序 | 通过（静态） | 66–73 行，72 行 `list_add(prev)` |
| 前后合并 | 通过（静态） | 111–120、122–131 行 |
| 计数 `nr_free` | 通过（静态） | 30、74、92、134–136 行 |
| 两个框架测试不变 | 通过（逐字节） | baseline 8–138 ↔ submission 139–269 |
| `page_init` 不变 | 通过（逐字节） | `pmm.c` 64–123 行 |
| `pmm.c` 仅一行选择 | 通过 | 37 行唯一改动 |
| `tools/`、`Makefile`、`entry.S`、`pmm.h` 不变 | 通过（sha256） | 见上表 |
| 编译 | 通过（外部证据） | `build.exitcode=0` |
| 有效 `make grade` | **未取得** | QEMU 7.0 启动假阴性；需 QEMU 4.1.1 复跑 |
| 阻断性代码缺陷 | 未发现 | 静态推演 + 编译通过 |
| 报告 | 未填写 | 模板 5518 字节，与 baseline 相同 |

**总体判定：代码静态合格，可以作为本地交付准备；运行验证尚未完成，不能把 0/20 或历史 PASS 当作本次真实 grade。** 本次未 commit/push，也未改动任何文件。
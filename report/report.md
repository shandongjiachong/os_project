# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 1：比麻雀更小的麻雀（最小可执行内核） |
| **小组成员** | 2412224-王泊凯、2413551-李睿、2411259-成钧翔 |
| **完成日期** | 2026-10-07 |

### 小组分工

本实验以已有代码为基础，重点在构建、运行、阅读和 GDB 验证，并未新增内核功能代码。实验共有两个练习，各组员均已在自己电脑上跑通，接着共同撰写实验报告，具体分工如下：

| 成员 | 负责内容 | 主要交付物 |
|------|---------|-----------|
| 2412224-王泊凯 | 练习一：内核入口静态分析 | `entry.S` 与 `kern_init` 的代码阅读和问答 |
| 2413551-李睿 | 练习二：GDB 启动流程验证 | `make debug`/`make gdb` 实测记录、截图 |
| 2411259-成钧翔 | 协助完成练习一二，以及实验报告整合撰写 | 实验报告 |

---

## 一、实验目的

<!-- 说明本次实验的主要目标和预期成果 -->

本实验的主要目的是：

1. 理解 RISC-V 机器从复位、OpenSBI 到操作系统内核的启动过程。
2. 理解链接脚本如何决定内核的加载地址、入口点和各段布局。
3. 理解内核启动栈的作用，并通过 GDB 验证 `sp` 的变化。
4. 理解 `la sp, bootstacktop` 和 `tail kern_init` 两条启动指令的作用。
5. 理解 SBI、`ecall`、控制台输出和 `cprintf` 之间的封装关系。
6. 掌握使用 GNU Make、QEMU 和 RISC-V GDB 编译、运行和调试最小内核的方法。

---

## 二、实验环境

你们使用的 AI 工具

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2412224-王泊凯 | Codex | DeepSeek-V4-Pro | 无 |
| 2413551-李睿 | VS Code | DeepSeek-V4-Pro | 无 |
| 2411259-成钧翔 | Codex | GPT-5 | 无 |


---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

lab1 围绕“一个最小内核如何被加载并开始运行”展开。实验暂时去掉复杂的
进程、内存管理和文件系统功能，只保留操作系统启动所必需的最小骨架：

```text
QEMU 复位
  -> 复位地址 0x1000
  -> OpenSBI 初始化
  -> 内核加载到 0x80200000
  -> kern_entry 设置内核栈
  -> kern_init 初始化 C 语言环境
  -> cprintf 输出启动信息
  -> 进入死循环
```

这个过程解决的核心问题是：在没有现成操作系统和标准 C 运行时支持的情况下，如何让一段 RISC-V 内核代码从固定地址启动，建立最基本的执行环境，并通过SBI 向控制台输出信息。

### 3.2 功能的逐步实现

1. 首先通过 `tools/kernel.ld` 指定内核的架构、入口点和加载地址，先确定内存布局，OpenSBI 再把内核交接到正确的位置。
2. 接着由 `kern/init/entry.S` 定义 `kern_entry`，设置启动栈顶，并跳转到 C 语言函数 `kern_init`。
3. `kern_init` 清零 `.bss` 段，为全局未初始化数据建立符合 C 语言约定的初始状态。
4. SBI 层通过 `ecall` 提供最底层的字符输出服务，控制台层和格式化输出层在此基础上逐层封装出 `cprintf`。
5. 最后使用 `make qemu` 验证内核能够启动并输出信息，再使用 GDB 从 `0x1000` 跟踪到 `0x80200000`，验证启动流程和寄存器变化。

---

## 四、实验内容与实现

### 练习一：理解内核启动中的程序入口操作

**负责人：** 王泊凯

#### 1. `la sp, bootstacktop` 完成了什么操作？

`la` 是 load address 的伪指令，它将符号 `bootstacktop` 的绝对物理地址加载到堆栈指针寄存器 `sp` 中。

它不是一条真实指令：汇编器把它展开成 PC 相对的两条指令。用 `riscv64-unknown-elf-objdump -d bin/kernel` 查看本次构建的反汇编，结果如下：

```asm
80200000:  00003117    auipc  sp,0x3
80200004:  00010113    mv     sp,sp
```

`auipc sp,0x3` 把当前 PC（`0x80200000`）加上 `0x3 << 12 = 0x3000`，得到 `0x80203000`；第二条本质是 `addi sp,sp,0`，用于补足地址的低 12 位。因为 `bootstacktop = 0x80203000` 的低 12 位恰好为 0，反汇编才显示成 `mv sp,sp`。两条执行完后 `sp = 0x80203000`，与符号表中的值一致：

```text
0000000080203000 D bootstacktop
```

#### 2. `la sp, bootstacktop` 的目的是什么？

在这条指令之前，`sp` 指向的仍是 OpenSBI 固件自己的栈（练习二实测交棒瞬间 `sp = 0x8001bd80`），内核不能继续借用固件的栈，必须建立自己的 C 语言执行环境：`kern_init` 及其调用的函数需要一块可用的栈来保存返回地址 `ra`、局部变量和被调用者保存寄存器，否则函数调用会访问无效地址。

需要说明的是，交棒瞬间的 `sp` 只代表固件当时使用的栈位置，与机器和 OpenSBI 版本有关（例如另一台机器上实测为 `0x8003def0`），而 `la sp, bootstacktop` 执行后的 `0x80203000` 只由源码和链接脚本决定，在任何机器上都相同。

RISC-V 的栈向低地址方向增长，因此栈指针的初值要取预分配栈区间的最高地址。`bootstack` 到 `bootstacktop` 正好是保留的两个页面：

```text
bootstacktop - bootstack = 0x80203000 - 0x80201000 = 0x2000 = 8192 字节
KSTACKSIZE = KSTACKPAGE(2) × PGSIZE(4096) = 8192 字节
```

其中 `KSTACKPAGE = 2`、`KSTACKSIZE = KSTACKPAGE * PGSIZE` 定义在 `kern/mm/memlayout.h`，`PGSIZE = 4096` 定义在 `kern/mm/mmu.h`；符号表里 `bootstacktop - bootstack = 0x2000` 正好等于这两个宏算出的 8192 字节，源码与实测互为印证。

这也解释了为什么初值取 `bootstacktop` 而不是 `bootstack`：只有从最高端开始，向下增长的栈才完整落在预留区间内。

#### 3. `tail kern_init` 完成了什么操作？

`tail kern_init` 是尾调用伪指令，含义是"跳转到目标函数并且不会再返回"。在 `objdump` 的反汇编里，它被汇编成一条 2 字节的压缩跳转指令：

```asm
80200008:  a009    j  8020000a <kern_init>
```

它把程序控制流无条件转移到 `kern_init`，既不修改返回地址寄存器 `ra`，也不建立新的调用栈帧。由于这条跳转只占 2 字节，`kern_init` 的入口落在 `0x8020000a`，而不是四字节对齐的地址。

#### 4. `tail kern_init` 的目的是什么？

`kern_init` 被声明为 `noreturn`，并且最后进入 `while (1)` 无限循环，永远不会返回 `kern_entry`。因此 `kern_entry` 无需保存返回地址、也无需建立调用栈帧；使用尾调用可以省去一个永远不会被使用的返回地址，让内核入口直接把控制权交给真正的 C 语言初始化函数。

#### 补充：`tools/kernel.ld` 如何决定内核的内存布局

- `OUTPUT_ARCH(riscv)` 指定输出文件的体系结构为 RISC-V；
- `ENTRY(kern_entry)` 指定 ELF 的入口符号，链接器据此填写 ELF 头的 `Entry point address`，用 `readelf -h bin/kernel` 可验证为 `0x80200000`；
- `BASE_ADDRESS = 0x80200000` 配合 `. = BASE_ADDRESS` 把定位计数器置到内核基地址，使 `.text` 段从 `0x80200000` 开始——这正是 OpenSBI 交棒时跳转的地址；
- 各段依次为 `.text`（代码）、`.rodata`（只读数据）、对齐到页边界后的 `.data`（已初始化数据，`bootstack` 就放在这里）、`.sdata`（gp 相对寻址的小数据）、`.bss`（未初始化数据）；
- `PROVIDE(etext = .)`、`PROVIDE(edata = .)`、`PROVIDE(end = .)` 把段边界导出为符号供 C 代码使用，`kern_init` 中的 `memset(edata, 0, end - edata)` 就依赖它们；
- `/DISCARD/` 丢弃 `.eh_frame` 和 `.note.GNU-stack`，这两个段在内核环境里没有用途。

#### 练习一的验证证据：符号地址与反汇编

执行：

```bash
riscv64-unknown-elf-nm -n bin/kernel | grep -E 'kern_entry|kern_init|bootstack$|bootstacktop|edata$|end$'
riscv64-unknown-elf-readelf -h bin/kernel | grep -E 'Entry|Type|Machine'
riscv64-unknown-elf-objdump -d bin/kernel | sed -n '1,26p'
```

关键结果：

```text
0000000080200000 T kern_entry
000000008020000a T kern_init
0000000080201000 D bootstack
0000000080203000 D bootstacktop
0000000080203008 D edata
0000000080203008 D end

Entry point address: 0x80200000

80200000:  00003117    auipc  sp,0x3
80200004:  00010113    mv     sp,sp
80200008:  a009        j      8020000a <kern_init>
```

六个符号与链接脚本、`mmu.h`、`memlayout.h` 中的定义互相印证：`kern_entry` 位于链接脚本指定的基地址 `0x80200000`，与 ELF 头的 `Entry point address` 一致；`kern_init = 0x8020000a` 正好是 `tail kern_init` 展开后的跳转目标 `j 8020000a`，符号表与反汇编互相印证；`bootstacktop - bootstack = 0x2000 = 8192` 字节，等于 `KSTACKPAGE(2) × PGSIZE(4096)`；`edata` 与 `end` 重合，说明本次构建的 `.bss` 段为空，`kern_init` 中 `memset(edata, 0, end - edata)` 的长度为 0。反汇编中 `addi sp,sp,-16 # 80202ff0 <bootstack+0x1ff0>` 进一步说明 `kern_init` 的栈帧确实落在 `bootstack` 预留区间内。

三个关键地址的含义：

- `0x80200000`：链接脚本指定的内核基地址，也是内核入口 `kern_entry` 的地址，即 ELF 头的 `Entry point address`；
- `0x80201000`：启动栈 `bootstack` 的起始地址，位于 `.data` 段；
- `0x80203000`：启动栈栈顶 `bootstacktop`，也就是 `kern_entry` 执行 `la sp, bootstacktop` 之后 `sp` 的取值。

符号地址由源码和链接脚本决定，与运行环境无关：只要源码和链接脚本不变，在任何机器上编译出的 `kern_entry`、`kern_init`、`bootstack`、`bootstacktop`、`edata`、`end` 都落在同样的地址，因此本节的符号表与反汇编可以和报告其他章节在不同机器上取得的运行截图并存。

![符号地址验证](images/symbols.png)

---

### 练习二：使用 GDB 验证启动流程

本阶段从 CPU 复位开始，用 GDB 逐步跟踪，验证内核如何从复位地址 `0x1000` 出发，经过 OpenSBI 最终进入 C 函数 `kern_init`，并确认`pc` 与 `sp` 的变化是否符合预期。

下面按启动流程的先后顺序逐步展开描述，关于本验证的具体实验截图见5.3、5.4小节。


#### 准备：建立 GDB 调试环境

使用命令（两个终端分别执行）：

```bash
# 终端一：启动 QEMU 并等待调试器连接
make debug

# 终端二：启动 GDB 并连接 QEMU
make gdb
```

如下述结果所示，`make debug` 让 QEMU 以调试模式启动，监听 GDB 端口，并让虚拟 CPU 在复位后立即暂停；`make gdb` 则启动`riscv64-unknown-elf-gdb` 并完成连接。连接成功时程序计数器停在 `0x1000`，说明此时 CPU 尚未进入 OpenSBI，更未进入内核。

```text
0x0000000000001000 in ?? ()
```

#### 步骤 1：查看复位代码

首先执行以下命令查看加电后的复位代码：

```gdb
(gdb) x/5i $pc
```

得到结果如下，`x/5i $pc` 显示的是 QEMU 硬编码在 `0x1000` 处的复位代码。它先把 `pc`存入 `t0`，再计算设备树地址并写入 `a1`，读取当前硬件线程号 `mhartid`到 `a0`，最后从 `0x1018` 加载固件加载地址到 `t0` 并跳转。这说明 CPU加电后确实从 `0x1000` 开始执行，而不是直接进入内核。

```asm
=> 0x1000:  auipc  t0, 0x0
   0x1004:  addi   a1, t0, 32
   0x1008:  csrr   a0, mhartid
   0x100c:  ld     t0, 24(t0)
   0x1010:  jr     t0
```

#### 步骤 2：单步执行，观察固件入口准备

本步骤执行的相关指令如下：

```gdb
(gdb) si
(gdb) si
(gdb) p/x $a1
(gdb) si
(gdb) p $a0
(gdb) si
(gdb) p/x $t0
```

得到结果：

```text
(gdb) p/x $a1
$1 = 0x1020
(gdb) p $a0
$2 = 0
(gdb) p/x $t0
$3 = 0x80000000
```

两次 `si` 后，`addi a1, t0, 32` 使 `a1 = 0x1020`，即设备树所在地址；`csrr a0, mhartid` 使 `a0 = 0`，表示当前为 0 号硬件线程；`ld t0, 24(t0)`则把 `t0` 更新为 `0x80000000`。这些寄存器为下一步跳转到 `0x80000000`处的 OpenSBI 做好了准备。

#### 步骤 3：跳转到 OpenSBI

接着再进行单步执行：

```gdb
(gdb) si
(gdb) p/x $pc
```

得到结果如下，执行 `jr t0` 后，`pc` 变为 `0x80000000`，这正是 OpenSBI 固件所在的物理内存区域。至此完成“复位 → OpenSBI”的跳转，后续的机器级初始化由OpenSBI 完成。

```text
$4 = 0x80000000
```

#### 步骤 4：在内核入口设置断点并继续执行、打印寄存器

接着使用以下指令在内核入口处设置断点，并继续执行：

```gdb
(gdb) b *0x80200000
(gdb) c
(gdb) info registers
```

得到以下结果，可以发现，OpenSBI 完成初始化后，把控制权交给链接脚本指定的内核入口`0x80200000`（即符号 `kern_entry`）。断点命中说明“复位 → OpenSBI → 内核”的前半段启动链已经打通。此时 sp 还是 OpenSBI 的栈顶，内核自己的栈尚未建立。

```text
Breakpoint 1, 0x0000000080200000 in ?? ()
pc  0x80200000
sp  0x8001bd80
```

#### 步骤 5：单步执行 `la sp, bootstacktop`

本步骤使用如下命令来单步执行`la sp, bootstacktop`：

```gdb
(gdb) si
(gdb) p/x $sp
```

得到结果：

```text
$5 = 0x80203000
```

执行 `la sp, bootstacktop` 后，`sp` 变为 `0x80203000`。`bootstacktop` 是链接脚本为启动栈 `bootstack` 预留空间的栈顶地址，这一步让内核拥有了自己的启动栈，为调用 C 函数`kern_init`准备好了栈空间。

#### 步骤 6：单步执行 `tail kern_init`

执行指令：

```gdb
(gdb) si
(gdb) p/x $pc
```

得到如下结果，`tail kern_init` 被汇编为一条无条件跳转 `j 0x8020000a <kern_init>`，直接把控制流交给 `kern_init`。它不保存返回地址、不建立新的调用栈帧，这与`kern_init` 被声明为 `noreturn` 的语义一致——内核初始化后进入死循环，不会返回到 `kern_entry`。

```gdb
$6 = 0x80200008
```

综上，GDB 实际验证的完整启动链为：

```text
0x1000（QEMU 复位）
  -> 0x80000000（OpenSBI 固件）
  -> 0x80200000（kern_entry，内核入口）
  -> sp = 0x80203000（la sp, bootstacktop）
  -> j 0x8020000a（tail kern_init）
  -> 0x8020000a（kern_init，C 初始化）
```

#### 核心问题回答

**一、这几条指令位于什么地址？**

处理器复位后，程序计数器 `pc` 被硬件设置为固定的复位向量。在 QEMU 模拟的 RISC-V 机器上，该复位地址为 `0x1000`。实测中 GDB 一连接就停在`pc = 0x1000`，`x/5i $pc` 显示出的正是 QEMU 硬编码的复位代码：

```asm
=> 0x1000:  auipc  t0, 0x0
   0x1004:  addi   a1, t0, 32
   0x1008:  csrr   a0, mhartid
   0x100c:  ld     t0, 24(t0)
   0x1010:  jr     t0
```

因此，RISC-V 硬件加电后最初执行的几条指令位于以 `0x1000` 为起点的复位代码区域（`0x1000` ~ `0x1014`）。

**二、这几条指令分别完成了什么功能？**

1. `auipc t0, 0x0`：`auipc`（add upper immediate to pc）把 20 位立即数左移 12 位后与当前 `pc` 相加，结果写入 `t0`。此处立即数为 0，所以`t0 = pc = 0x1000`，从而把当前执行地址作为后续访存的基址。

2. `addi a1, t0, 32`：计算 `a1 = t0 + 32 = 0x1020`。`0x1020` 是设备树（FDT，Flattened Device Tree）所在的地址，它通过 `a1` 作为参数传给后续的固件，用于向 OpenSBI 描述机器的硬件信息。

3. `csrr a0, mhartid`：`csrr` 把机器态控制状态寄存器 `mhartid`（hardware thread ID，硬件线程编号）读入 `a0`，得到当前 hart 的编号。在单核（主核）情况下 `mhartid = 0`。它作为第一个参数传给固件，用于标识当前 hart。

4. `ld t0, 24(t0)`：从内存地址 `t0 + 24 = 0x1018` 处加载一个 8 字节值到`t0`。`0x1018` 紧跟在复位代码之后，其中存放的是固件的加载地址；本次实测中 `t0` 被更新为 `0x80000000`，即 OpenSBI 固件的入口。

5. `jr t0`：无条件跳转到 `t0` 中的地址`0x80000000`，把控制权交给OpenSBI 固件，至此完成“复位 → OpenSBI”的交接。

**三、整体功能总结**

这五条指令是 QEMU 为 RISC-V 机器硬编码的一段最小引导代码，其整体作用是为跳转到 OpenSBI 固件做好两项准备：一是通过 `a0`、`a1` 传入 `hart id`和设备树地址两个参数，二是从固定位置 `0x1018` 取得固件入口地址，最终
用 `jr` 跳转过去。它不涉及任何内核代码，只是操作系统启动链中从 CPU 复位到固件运行之间的最前序环节。

---

## 五、测试与验证

### 1. 编译验证

执行：

```bash
make clean
make
```

构建成功，生成：

- `bin/kernel`：带调试信息的 ELF 内核文件；
- `bin/ucore.img`：供 QEMU 加载的二进制内核镜像。

![编译结果](images/gdb08.png)

### 2. QEMU 验证

执行：

```bash
make qemu
```

OpenSBI 正常启动，出现启动信息后，内核进入 `while (1)` 死循环，符合实验预期。

![QEMU 运行结果](images/gdb09.png)

### 3. GDB 验证

`make debug` 和 `make gdb` 成功建立远程调试连接。

![GDB 内核入口](images/gdb07.png)

![GDB 内核入口](images/gdb01.png)

### 4. 启动流程验证截图

GDB 成功从 `pc = 0x1000` 跟踪到 `kern_entry = 0x80200000`，并验证了
`sp = 0x80203000`。

![GDB 内核入口](images/gdb02.png)

![GDB 内核入口](images/gdb03.png)

![GDB 内核入口](images/gdb04.png)

![GDB 内核入口](images/gdb05.png)

![GDB 内核入口](images/gdb06.png)

---

## 六、实验总结与收获

### 对操作系统的理解

#### 1. 本实验中的重要知识点

本实验覆盖的主要知识点如下：

1. **OpenSBI 与内核交接** 对应 OS 原理中的“固件、引导程序和操作系统启动链”。固件（OpenSBI）先初始化机器，再把控制权以 S 态交给内核，这是计算机从加电到运行操作系统的一整条启动链在本实验中的缩影。

2. **复位地址 `0x1000`** 对应“CPU 上电/复位后的执行入口”。处理器复位后需要一个固定起点来执行第一条指令，本实验中该起点就是 QEMU 硬编码的`0x1000`。

3. **链接脚本** 对应“程序装载地址和内存布局”。它决定代码段、只读数据段、数据段和 BSS 段各自放在哪里，从而决定内核入口 `0x80200000` 和栈顶`0x80203000`。

4. **内核栈** 对应“函数调用和执行上下文”。它为 C 函数的调用、局部变量和寄存器现场保存提供空间，`la sp, bootstacktop` 就是建立内核栈的关键一步。

5. **GDB 远程调试** 对应“内核调试方法”。GDB 借助 QEMU 的远程调试能力暂停虚拟 CPU，从而观察 `pc`、`sp` 和逐条指令的执行过程。

#### 2. OS 原理中重要但本实验尚未涉及的知识

本实验还没有实现：

- 页表、地址空间和虚拟内存；
- 物理页分配和动态内存管理；
- 异常、中断和系统调用；
- 线程、进程和上下文切换；
- CPU 调度和时间片；
- 锁、信号量和并发控制；
- 用户态程序和用户/内核态切换；
- 文件系统、磁盘驱动和缓存。

### AI 协作开发的经验

1. 先让 AI 阅读手册和代码结构，再执行命令，能够减少盲目修改。
2. 遇到环境问题时，要区分代码错误、工具版本差异和权限问题。
3. AI 给出的命令必须结合实际输出验证，不能只凭预期结果填写报告。
4. GDB 调试时应每次只执行一个明确动作，并记录 `pc`、`sp`、`ra` 等关键寄存器，才能把汇编指令和 C 代码执行流程对应起来。
---


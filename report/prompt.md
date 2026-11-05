# Lab3 提示词汇总

本文件用于汇总本实验使用过的 prompt。

## 功能模块一：Trap 上下文恢复与 Trap 入口功能

负责人：2413551-李睿

```

[PROMPT]

**任务**：
修改 `kern/trap/trapentry.S`，实现 Lab3「Trap 上下文恢复与 Trap 入口」功能模块。
本模块负责 Trap 从"进入内核"到"返回原执行流"之间的底层上下文管理，包含两件事：
1. 实现汇编宏 `RESTORE_ALL`，把当前内核栈顶 Trap frame 中保存的处理器状态恢复回通用寄存器与
   相关 CSR，使 `__trapret` 末尾的 `sret` 能正确返回被打断的执行流；
2. 补全 `__alltraps` 中 `SAVE_ALL` 之后、`__trapret` 之前的调用代码，建立汇编 Trap 入口与
   C 语言处理函数 `trap(struct trapframe *tf)` 之间的调用关系，把 Trap frame 按 RISC-V
   调用约定作为参数传递出去，并在 `trap()` 返回后继续走到 `__trapret`。
本模块只负责"保存态 → 恢复态"和"汇编 → C"的衔接，**不负责**中断/异常的具体分类与处理。

**操作要求**：
- 你必须在项目中修改实际文件 `kern/trap/trapentry.S`，而不是仅展示代码片段。
- 只在文件中已有的 `#lab3 function1 YOUR CODE` 标记位置补全代码，保留文件中的其余一切现有
  内容；保留 `#lab3 function1` 注释本身，并把同一行的 `YOUR CODE` 替换为学号 `<2413551>`。
- 不得修改 `SAVE_ALL` 宏、`struct trapframe` 定义、`void trap(struct trapframe *tf)` 的函数
  签名，不得修改 `__alltraps` / `__trapret` 这两个符号名，不得删除 `sret`。
- 不得修改 `kern/trap/trap.c`、`kern/driver/*`、`kern/init/init.c` 等其他文件；本任务
  不涉及 `interrupt_handler()` / `exception_handler()` 的实现。
- 允许新增仅在本文件内部使用的辅助汇编宏，但不得改变任何公开符号和已有数据布局。

**输出要求**：
直接进行文件操作。使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 部分提供的信息；
一切以工程中真实存在的宏名、结构体字段和偏移为准，不要凭记忆引入不存在的宏或自行更改
trapframe 布局，不要引入与本模块无关的功能（如用户态栈切换、进程调度、页表切换）。


[RELY]

// 1) 汇编层可用的宏（来自 libs/riscv.h，已在 trapentry.S 顶部通过 #include <riscv.h> 引入）
//    __riscv_xlen == 64 时：
//      STORE  => sd
//      LOAD   => ld
//      REGBYTES => 8        （LOG_REGBYTES = 3）
//    因此 36 * REGBYTES = 288 字节，恰好是 16 的整数倍（不破坏栈的 16 字节对齐）

// 2) 数据结构（来自 kern/trap/trap.h）——Trap frame 在栈上的布局必须与此完全一致
struct pushregs {
    uintptr_t zero, ra, sp, gp, tp, t0, t1, t2, s0, s1,
              a0, a1, a2, a3, a4, a5, a6, a7,
              s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
              t3, t4, t5, t6;          // 共 32 个 uintptr_t，顺序即 x0..x31 的 ABI 顺序
};
struct trapframe {
    struct pushregs gpr;              // 偏移 0*8  ~ 31*8
    uintptr_t status;                 // 偏移 32*8，对应 sstatus
    uintptr_t epc;                    // 偏移 33*8，对应 sepc
    uintptr_t badvaddr;               // 偏移 34*8，对应 sbadaddr
    uintptr_t cause;                  // 偏移 35*8，对应 scause
};
// 即 trapframe 共 36 个 REGBYTES；栈顶 sp 指向 trapframe 首地址（gpr.zero 处）。

// 3) 已有 SAVE_ALL 的关键行为（不可修改，RESTORE_ALL 必须与之严格对应）
    .macro SAVE_ALL
    csrw sscratch, sp                 // 把 Trap 发生前的 sp 暂存到 sscratch
    addi sp, sp, -36 * REGBYTES       // 栈顶下移一个 trapframe
    STORE x0,  0*REGBYTES(sp)         // x0 恒为 0
    STORE x1,  1*REGBYTES(sp)         // ra
    STORE x3,  3*REGBYTES(sp)         // gp  … 一直到 x31
    // … x4 ~ x31 依次存入 4*REGBYTES ~ 31*REGBYTES，唯独没有直接存 x2(sp)
    csrrw s0, sscratch, x0            // s0 = Trap 前的 sp；同时 sscratch 被清零
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, sbadaddr
    csrr s4, scause
    STORE s0, 2*REGBYTES(sp)          // ★ Trap 前的 sp 存在 2*REGBYTES 槽位
    STORE s1, 32*REGBYTES(sp)         // ★ sstatus
    STORE s2, 33*REGBYTES(sp)         // ★ sepc
    STORE s3, 34*REGBYTES(sp)         // ★ badvaddr
    STORE s4, 35*REGBYTES(sp)         // ★ scause
    .endm
// 结论：槽位 2 是"Trap 前的 sp"，而不是 Trap 发生瞬间 sp 的值；槽位 0 是 0。
//      SAVE_ALL 已经把 sscratch 清零，符合本实验"内核态 Trap 时 sscratch == 0"的约定，
//      trapframe 中不存在 sscratch 槽位。

// 4) 当前文件骨架（待补全位置）
    .globl __alltraps
    .align(2)                         // 必须 4 字节对齐（stvec Direct 模式要求）
__alltraps:
    SAVE_ALL
    #lab3 function1 YOUR CODE         ← 需要在这里补"调用 trap(tf)"的代码
    # sp should be the same as before "jal trap"
    .globl __trapret
__trapret:
    RESTORE_ALL                       ← RESTORE_ALL 宏体需要实现
    sret

// 5) C 侧接口（不可修改）
void trap(struct trapframe *tf);      // trap() 返回 void，处理完直接返回

// 6) RISC-V 调用约定与 Trap 返回语义
//    - 第一个整型参数通过 a0(x10) 传递；调用者可用 jal/jalr/call 建立返回地址 ra(x1)。
//    - a0~a7、t0~t6 为 caller-saved，`trap()` 返回后其值不可依赖；
//      ra/gp/tp/sp/s0~s11 为 callee-saved。
//    - 调用 `trap()` 时 sp 已指向 trapframe，且 288 字节是 16 的倍数，
//      满足 ABI 的 16 字节栈对齐要求。
//    - sret 的行为：pc ← sepc；特权级 ← sstatus.SPP；SIE ← SPIE；SPIE ← 1；SPP ← 0。
//      因此 sepc / sstatus 必须在 sret 之前写回 CSR，否则会返回到错误的地址或状态。

// 7) 可验证的验收背景（用于校准行为，不要求你编写测试）
//    tools/test_lab3_context.S 先把 t0~t2、s0~s1、a1~a7、s2~s11、t3~t6（共 35 个寄存器）
//    置为 0x105~0x11f，执行 ebreak 触发断点异常，返回后逐一比较这些寄存器是否仍为原值
//    （gp、tp 保持不变，不做检查）。另有测试要求断点/非法指令异常处理后 epc 被推进，
//    即返回位置必须以 trapframe 中（可能被修改的）epc 为准。


[GUARANTEE]

必须实现或补全的内容（均在 `kern/trap/trapentry.S`）：

```asm
    .macro RESTORE_ALL
        # 与 SAVE_ALL 严格互逆的上下文恢复
    .endm

__alltraps:
    SAVE_ALL
    # 补全：把当前 Trap frame 作为参数传给 trap()，返回后 sp 仍指向同一 Trap frame
    .globl __trapret
__trapret:
    RESTORE_ALL
    sret
```

你可以根据需要添加：
- 仅在本文件内部使用的辅助汇编宏（不得改变已有符号名、不得改变 trapframe 布局）
- 局部标签（避免与工程中其他汇编文件重名）

不允许：
- 修改 `SAVE_ALL`、`struct trapframe`、`trap()` 签名、`__trapret` 与 `sret`
- 在 trapframe 中新增字段或改变任意槽位含义
- 修改其他源文件


[SPECIFICATION]

## RESTORE_ALL

**Pre-Condition**：
- 已执行过 `SAVE_ALL`，且 sp 仍指向本次 Trap 的 Trap frame 首地址（未被 `trap()` 破坏）。
- CSR 中仍保留 Trap 发生瞬间的 `sstatus` / `sepc` 旧值（可能已被 C 代码通过
  `tf->epc` 修改，因此 CSR 中的值不一定是最新的返回目标）。
- 通用寄存器内容此时已无保留价值（都可以被覆盖或从 frame 重新装入）。

**Post-Condition**：
- 全部 32 个通用寄存器恢复到 Trap frame 中记录的值，唯一例外是 x2(sp) 恢复为槽位 2 中
  记录的"Trap 前的 sp"，x0 仍为 0。
- `sstatus` 与 `sepc` 两个 CSR 被写成 Trap frame 中 32 / 33 槽位的当前值，使紧随其后的
  `sret` 能返回到正确位置与正确状态。
- 恢复完成后 sp 指向 Trap 发生前的栈顶，`sret` 之后执行流无缝继续。
- 除上述 CSR 外，本模块不改变 `sscratch`（保持 SAVE_ALL 之后的 0，符合内核态约定）。

**Case 1**（正常路径，Trap 来自内核态 S 模式，SPP = 1）：
按槽位把 x1、x3~x31 依次装入通用寄存器，把 32/33 槽位写回 `sstatus`、`sepc`，最后把
2 号槽位的值装入 sp；此过程结束后要求所有被检查寄存器与 Trap 前完全一致。

**Case 2**（C 代码修改了返回位置）：
`trap()` 可能已把 `tf->epc` 改成"触发异常指令的下一条指令"的地址。恢复时必须使用
Trap frame 中 33 号槽位的当前值写 `sepc`，而不是延用 CSR 中的旧值，否则会重复执行
同一条异常指令形成死循环。

**Case 3**（sp 的恢复顺序）：
sp 既是恢复目标又是所有装载操作的基址寄存器，因此 sp 只能作为最后一步恢复；在恢复 sp
之前，任何以 sp 为基址的装载都必须仍然成立。

**Case 4**（临时寄存器的使用）：
若使用某个通用寄存器作为装载 CSR 值的临时寄存器（用于 `csrw sstatus` / `csrw sepc`），
该寄存器也必须在之后从 Trap frame 中恢复，不能被永久破坏。

**Requirements**：
- `RESTORE_ALL` 必须是与 `SAVE_ALL` 一一对应的逆操作，不得自行假设任何与工程实际
  trapframe 不一致的偏移；特别注意 x2(sp) 的保存方式与其他通用寄存器不同。
- 不得把 x2 当作普通槽位按相同方式提前恢复。
- 恢复过程不得依赖 `trap()` 返回后 a0~a7、t0~t6 的值（它们是 caller-saved）。
- 不得写入 trapframe 中不存在的槽位（如 sscratch 槽位）。
- 恢复后 sp 必须回到 Trap 发生前的值，正是它保证了 `sret` 之后原执行流继续。

## __alltraps（`SAVE_ALL` 之后、`__trapret` 之前的部分）

**Pre-Condition**：
- sp 指向由 `SAVE_ALL` 建好的完整 Trap frame，其中通用寄存器与 sstatus/sepc/badvaddr/cause
  均已落栈。

**Post-Condition**：
- `trap(struct trapframe *tf)` 被调用且参数 tf 等于当前 Trap frame 的首地址（即
  `SAVE_ALL` 之后的 sp 值）。
- `trap()` 返回后，控制流顺序进入 `__trapret`，并且 sp 仍等于调用前的 Trap frame 首地址，
  以便 `RESTORE_ALL` 从正确位置恢复。

**Case 1**（正常传递）：
以 Trap frame 首地址作为第一个整型参数传给 `trap()`，采用 RISC-V 的标准调用形式；
`trap()` 返回后不回退、不跳转，直接落到 `__trapret`。

**Case 2**（`trap()` 修改了 Trap frame）：
`trap()` 通过指针修改 `tf->epc` 等字段后返回，`__alltraps` 不需要做任何同步动作，
但必须保证 sp 未被改变，使这些修改能被 `RESTORE_ALL` 读到。

**Requirements**：
- 必须保持 `SAVE_ALL` / `__alltraps` / `__trapret` 的 4 字节对齐要求（`stvec` Direct 模式）。
- 传递给 `trap()` 的参数必须来自"当前 sp"，不能是全局变量或寄存器中被覆盖的残留值。
- 调用点必须满足 16 字节栈对齐（288 字节 frame 已满足，不要额外破坏）。
- 不得在调用前后修改 trapframe 的内容。
- 不得省略 `__trapret` 的落地路径；`__trapret` 与 `sret` 保持不变。

## 端到端组合行为

**Pre-Condition**：`idt_init()` 已把 `stvec` 设为 `&__alltraps`（Direct 模式），
`sscratch` 为 0，Trap 由内核态指令（如 `ebreak`、非法指令、时钟中断）触发。

**Post-Condition**：
一次完整 Trap 满足 `__alltraps → SAVE_ALL → trap(tf) → __trapret → RESTORE_ALL → sret`
闭环；返回后 pc 为 Trap frame 中 epc 的最终值，特权级与 SIE 由 sstatus 恢复，
通用寄存器内容与 Trap 发生前一致。

```
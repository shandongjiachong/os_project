#include <proc.h>
#include <kmalloc.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <vmm.h>
#include <trap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* ------------- process/thread mechanism design&implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  ucore implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in lab6), etc. ucore needs to
manage all these details efficiently. In ucore, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc,
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:

  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  +
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit
SYS_getpid      : get the process's pid

*/

// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT 10
#define HASH_LIST_SIZE (1 << HASH_SHIFT)
#define pid_hashfn(x) (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

// idle proc
struct proc_struct *idleproc = NULL;
// init proc
struct proc_struct *initproc = NULL;
// current proc
struct proc_struct *current = NULL;

static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);

// set_proc_name - set the name of proc
char *
set_proc_name(struct proc_struct *proc, const char *name)
{
    memset(proc->name, 0, sizeof(proc->name));
    return memcpy(proc->name, name, PROC_NAME_LEN);
}

// get_proc_name - get the name of proc
char *
get_proc_name(struct proc_struct *proc)
{
    static char name[PROC_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, proc->name, PROC_NAME_LEN);
}

// get_pid - alloc a unique pid for process
static int
get_pid(void)
{
    static_assert(MAX_PID > MAX_PROCESS);
    struct proc_struct *proc;
    list_entry_t *list = &proc_list, *le;
    static int next_safe = MAX_PID, last_pid = MAX_PID;
    if (++last_pid >= MAX_PID)
    {
        last_pid = 1;
        goto inside;
    }
    if (last_pid >= next_safe)
    {
    inside:
        next_safe = MAX_PID;
    repeat:
        le = list;
        while ((le = list_next(le)) != list)
        {
            proc = le2proc(le, list_link);
            if (proc->pid == last_pid)
            {
                if (++last_pid >= next_safe)
                {
                    if (last_pid >= MAX_PID)
                    {
                        last_pid = 1;
                    }
                    next_safe = MAX_PID;
                    goto repeat;
                }
            }
            else if (proc->pid > last_pid && next_safe > proc->pid)
            {
                next_safe = proc->pid;
            }
        }
    }
    return last_pid;
}


// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void
forkret(void)
{
    forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void
hash_proc(struct proc_struct *proc)
{
    list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}

// find_proc - find proc frome proc hash_list according to pid
struct proc_struct *
find_proc(int pid)
{
    if (0 < pid && pid < MAX_PID)
    {
        list_entry_t *list = hash_list + pid_hashfn(pid), *le = list;
        while ((le = list_next(le)) != list)
        {
            struct proc_struct *proc = le2proc(le, hash_link);
            if (proc->pid == pid)
            {
                return proc;
            }
        }
    }
    return NULL;
}


// setup_kstack - alloc pages with size KSTACKPAGE as process kernel stack
static int
setup_kstack(struct proc_struct *proc)
{
    struct Page *page = alloc_pages(KSTACKPAGE);
    if (page != NULL)
    {
        proc->kstack = (uintptr_t)page2kva(page);
        return 0;
    }
    return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void
put_kstack(struct proc_struct *proc)
{
    free_pages(kva2page((void *)(proc->kstack)), KSTACKPAGE);
}


// do_exit - called by sys_exit
//   1. call exit_mmap & put_pgdir & mm_destroy to free the almost all memory space of process
//   2. set process' state as PROC_ZOMBIE, then call wakeup_proc(parent) to ask parent reclaim itself.
//   3. call scheduler to switch to other process
int do_exit(int error_code)
{
    panic("process exit!!.\n");
}

// init_main - the second kernel thread used to create user_main kernel threads
static int
init_main(void *arg)
{
    cprintf("this initproc, pid = %d, name = \"%s\"\n", current->pid, get_proc_name(current));
    cprintf("To U: \"%s\".\n", (const char *)arg);
    cprintf("To U: \"en.., Bye, Bye. :)\"\n");
    return 0;
}

//LAB4 function2 YOUR CODE


//LAB4 function2 2412224
// 本部分（B）负责：内核线程的创建与登记
//   copy_mm()     —— 本阶段内核线程共享内核地址空间，只保留接口形态
//   copy_thread() —— 把中断帧与首次运行的上下文布置到新线程自己的内核栈上
//   do_fork()     —— 组合 PCB/内核栈/PID/链表/状态，真正造出一个可运行实体
//   kernel_thread() —— 把"函数入口 + 参数"包装成一个内核线程

// copy_mm - 处理新线程的内存管理信息
//  note: 实验四创建的线程都是内核线程，它们直接使用内核的地址空间，
//        所以 current->mm 为 NULL，本阶段不需要真正复制页表，只要接口完整。
static int
copy_mm(uint32_t clone_flags, struct proc_struct *proc)
{
    assert(current->mm == NULL);
    /* do nothing in this project */
    (void)clone_flags;
    (void)proc;
    return 0;
}

// copy_thread - 在新线程内核栈上布置中断帧，并设置它第一次被调度时的上下文
//  param proc: 新线程的 PCB（kstack 已由 setup_kstack 分配好）
//  param esp:  新线程第一次运行时的栈指针（内核线程传 0，稍后由中断帧决定）
//  param tf:   kernel_thread 构造的中断帧模板
//  note: 这里布置的 context.ra = forkret，配合 trapentry.S 中的
//        forkret -> forkrets -> __trapret -> kernel_thread_entry 这条链，
//        新线程第一次被 switch_to 切换过去时就能"像从中断返回一样"开始执行。
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf)
{
    // 中断帧放在内核栈的最顶端（栈向下增长，不会覆盖栈上其它数据）
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE - sizeof(struct trapframe));
    // 复制中断帧模板，随后修正其中几个只属于新线程的字段
    *(proc->tf) = *tf;

    // a0 存放返回值：子线程从 fork 返回 0
    proc->tf->gpr.a0 = 0;
    // 首次运行时的内核栈：RESTORE_ALL 会以 sp 为基准恢复现场
    proc->tf->gpr.sp = (esp == 0) ? (uintptr_t)proc->tf : esp;

    // 上下文切换后先回到 forkret，由它取出 tf 走中断返回的恢复链路
    proc->context.ra = (uintptr_t)forkret;
    proc->context.sp = (uintptr_t)(proc->tf);
}

// do_fork - 创建并登记一个新线程/进程
//  param clone_flags: 复制标志（CLONE_VM/CLONE_THREAD 等）
//  param stack:       新线程的用户栈（内核线程为 0）
//  param tf:          中断帧模板
//  return:            新线程的 pid；失败返回负错误码
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf)
{
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    bool intr_flag;

    // 进程数超过上限，直接失败
    if (nr_process >= MAX_PROCESS)
    {
        goto fork_out;
    }
    ret = -E_NO_MEM;

    // 1. 分配并初始化进程控制块
    if ((proc = alloc_proc()) == NULL)
    {
        goto fork_out;
    }
    // 子线程的父进程就是当前线程
    proc->parent = current;

    // 2. 分配内核栈（KSTACKPAGE 个连续物理页）
    if (setup_kstack(proc) != 0)
    {
        goto bad_fork_cleanup_proc;
    }

    // 3. 处理内存管理信息：本实验的内核线程共享内核地址空间
    if (copy_mm(clone_flags, proc) != 0)
    {
        goto bad_fork_cleanup_kstack;
    }
    // 内核线程都跑在同一张内核页表上，切换时 satp 直接指向 boot_pgdir_pa
    proc->pgdir = boot_pgdir_pa;

    // 4. 布置中断帧和首次运行上下文
    copy_thread(proc, stack, tf);

    // 5. 分配 pid，并挂入线性链表和哈希表（这几步必须原子完成）
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        list_add(&proc_list, &(proc->list_link));
        nr_process++;
    }
    local_intr_restore(intr_flag);

    // 6. 新线程进入就绪态，等待调度
    wakeup_proc(proc);

    // 7. 返回新线程的 pid
    ret = proc->pid;

fork_out:
    return ret;

// 失败路径：按"后申请的先释放"的顺序回收资源
bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}

// kernel_thread - 创建一个内核线程，让它去执行 fn(arg)
//  param fn:          线程入口函数
//  param arg:         传给入口函数的参数
//  param clone_flags: 复制标志
//  return:            新线程的 pid；失败返回负错误码
//  note: kernel_thread_entry（kern/process/entry.S）约定的寄存器用法是
//        s0 = 入口函数，s1 = 参数，所以这里把 fn/arg 放进中断帧模板的 s0/s1。
int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags)
{
    struct trapframe tf;
    // 系统启动初期没有可复制的父进程，用 memset 构造一个干净的中断帧模板
    memset(&tf, 0, sizeof(struct trapframe));

    // s0 存放线程入口，s1 存放参数（见 kernel_thread_entry）
    tf.gpr.s0 = (uintptr_t)fn;
    tf.gpr.s1 = (uintptr_t)arg;

    // 新线程第一次 sret 后应回到 S 态执行 kernel_thread_entry：
    //   SPP = 1  → 返回后处于内核态
    //   SPIE = 1 → sret 之后重新开中断
    //   SIE  = 0 → 当前这段构造代码里先关中断，切换过去后再打开
    tf.status = (read_csr(sstatus) & ~SSTATUS_SIE) | SSTATUS_SPP | SSTATUS_SPIE;
    tf.epc = (uintptr_t)kernel_thread_entry;

    // 交给 do_fork 去完成 PCB、内核栈、登记等工作
    return do_fork(clone_flags, 0, &tf);
}

//LAB4 function3 YOUR CODE

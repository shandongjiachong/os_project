# Lab3：A 完成后的阶段代码

本目录是正式课程框架的 A 阶段副本，保留了以下实现：

- `kern/mm/pmm.c`：接入 Lab2 的 `page_init()` 和 Best-Fit 选择，保留 Lab3 的中断保护。
- `kern/mm/best_fit_pmm.c`：接入 Lab2 的 Best-Fit，保留课程自带测试。
- `kern/trap/trapentry.S`：补完 `RESTORE_ALL` 和 `__alltraps` 的 C 调用，兼容当前汇编器的 `stval` CSR 名称。

`kern/trap/trap.c` 已恢复课程的 `lab3 function2 YOUR CODE` 骨架，供 B、C 接着实现中断和异常处理。因此该阶段不能完整构建或运行评分；课程测试及评分脚本保持原样，Lab3 学号仍待实际负责人填写。

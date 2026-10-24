# B 部分验证证据索引

**最终结果：QEMU 4.1.1 下普通分配器自检成功，原版 make grade 退出 0、两项 OK、20/20，同次评分串口两个 PASS 均保存。** 没有修改启动链、page_init、框架测试或评分脚本。旧失败批次保持原样，不能混作最终通过证据。

## 1. 最终完整批次：final-20261024

运行日期：2026-10-24（Asia/Shanghai）。QEMU 普通自检观察于 11:57:28 结束；评分于 11:59:37 结束，时间由真实截图记录。

| 阶段 | 实际结果 | 完整证据 |
|---|---|---|
| clean | 退出 0 | [命令](<final-20261024/clean.command.log>)、[日志](<final-20261024/clean.log>)、[退出码](<final-20261024/clean.exitcode>) |
| build | 退出 0 | [命令](<final-20261024/build.command.log>)、[日志](<final-20261024/build.log>)、[退出码](<final-20261024/build.exitcode>) |
| 普通 QEMU | best_fit_pmm_manager，check_alloc_page() succeeded!；观察超时退出 124 | [命令](<final-20261024/qemu.command.log>)、[日志](<final-20261024/qemu.log>)、[退出码](<final-20261024/qemu.exitcode>)、[截图](<final-20261024/qemu-actual.png>) |
| 原版 make grade | Page Init OK、Best Fit OK、20/20、退出 0 | [命令](<final-20261024/grade.command.log>)、[日志](<final-20261024/grade.log>)、[退出码](<final-20261024/grade.exitcode>)、[同次串口](<final-20261024/grade-serial.log>)、[截图](<final-20261024/grade-actual.png>) |

同次评分串口最后两行为：

```text
[Test 1] Page Init: PASS
[Test 2] Best-Fit: PASS
```

普通 QEMU 内核常驻，外层 timeout 在 15 秒结束观察，124 不是内核自动正常退出，也不能单凭它判断通过；通过依据是实际自检标记。评分外层观察上限 120 秒，实际正常退出 0。Test 2 检查管理器指针，必须结合普通 best_fit_check 自检理解，不能夸大为所有边界情况均有动态测试。

截图来自真实 Windows Terminal/WSL 运行窗口，未经文字重绘、编辑或合成。评分截图同时显示实际 20/20、退出 0，以及随后读取的同一次评分串口。外部采集使用 script/tee/timeout 记录原命令，不替换 QEMU 参数；没有手工注入或放宽测试。GRADE_TMP 在 WSL /tmp，GRADE_QEMU_OUT 也在 WSL /tmp，评分完成后原样复制串口到本目录。命令以 command.log 为准，采集脚本存于[采集流程](<capture-scripts/final-phase.sh>)和[控制台流程](<capture-scripts/final-console.ps1>)。

![普通启动与自检真实截图](<final-20261024/qemu-actual.png>)

![原版评分20/20及同次两个PASS](<final-20261024/grade-actual.png>)

## 2. 环境及先前通过批次

- [QEMU 版本](<passing-114140/qemu-version.log>)、[编译器版本](<passing-114140/compiler-version.log>)：QEMU 4.1.1、OpenSBI v0.4、GCC 15.1.0。GCC 沿用获得[用户确认](<toolchain-confirmation-event.txt>)，未认定为 SiFive 官方发行。
- [QEMU独立安装路径](<qemu411-install/executable.txt>)、[构建退出码](<qemu411-install/build.exitcode>)、[安装前哈希](<qemu411-install/hashes-before.txt>)、[原QEMU安装后哈希](<qemu411-install/existing-qemu-after.txt>)。原有 QEMU 未覆盖。补存[当时执行的构建脚本](<qemu411-install/build-script.sh>)和[完整构建日志](<qemu411-install/build.log>)，仅规范复制文本的末尾换行；[Ubuntu版本核验](<qemu411-install/ubuntu-release.log>)直接确认 22.04.5 LTS。
- 11:41/11:42 批次：[启动日志](<passing-114140/boot-before-monitor.log>)、[构建](<passing-114140/build.log>)、[评分20/20](<passing-114140/grade.log>)、[评分结果](<passing-114140/grade-result.json>)、[评分退出码](<passing-114140/grade.exitcode>)。该批次用 monitor quit 结束观察，不是内核自动退出。
- 该批次[早期结果对象](<passing-114140/result.json>)的 grade 字段仍为当时 pending，不涂改原记录；后续评分以 grade-result.json 和 grade.log 为准。当次临时串口被原 Makefile 清理，不能补造 PASS。最终批次已独立补齐。
- [运行前源码哈希](<passing-114140/sources-before.json>)、[运行后源码哈希](<passing-114140/sources-after.json>)与最终审计核对当前受测源码。部分旧文本证据复制时规范了末尾换行；原始文件仍在隔离根验证目录。新最终批次日志保持采集字节。

## 3. 历史失败与采集失败

| 批次 | 事实与定位 |
|---|---|
| evidence 根目录旧主批次 | QEMU 7.0.0；build=0、qemu=124、grade=2，0/20。串口只到OpenSBI、Next Address=0；见[会话](<verification-session.log>)、[评分](<grade.log>)、[串口](<grade-serial.log>)。 |
| terminal-run | 旧环境复跑0/20；见[会话](<terminal-run/terminal-session.log>)与[评分](<terminal-run/grade.log>)。 |
| terminal-capture | 旧环境真实失败截图；[评分](<terminal-capture/grade.log>)与[截图](<terminal-capture/grade-actual.png>)。评分串口只到固件中途，不把别批次Next Address数字移入它。 |
| accepted-20261024 | 已换QEMU4.1.1，普通自检成功；评分输出指向Windows挂载盘时出现 grep: No data available，当次0/20、退出2。见[评分](<accepted-20261024/grade.log>)、[退出码](<accepted-20261024/grade.exitcode>)、[截断串口](<accepted-20261024/grade-serial.log>)。其中grade-actual.png仅标题、正文空白，不能充当成功截图。 |

最后一项改为 WSL /tmp 串口位置后复跑取得最终20/20；这是证据采集路径调整，不是改写评分或修改算法。没有对挂载盘报错的底层原因作未经证明的唯一归因。轻微时钟偏差警告保留，不隐藏。

历史文档见[原报告](<history-documents/说明-验收前.md>)、[原交接](<history-documents/交接.md>)、[原证据索引](<history-documents/evidence-README.md>)。它们的失败/待补表述只对应当时状态；归档中的相对链接是历史原文，不作为现行导航。

## 4. 源码、Prompt 与完整性

[基础归档核验](<baseline-verification.json>)、[范围审计](<scope-audit.json>)、[补丁](<B-best-fit.patch>)、[差异统计](<diff-stat.log>)、[最终核验](<final-validation.json>)、[文件哈希清单](<deliverable-manifest.json>)。相对基础归档只有 best_fit_pmm.c 与 pmm.c 的管理器选择变化，保护项保持不变。

[B原始分工](<B-assignment-original.txt>)、[提示索引](<../prompts.md>)、[48条真实用户记录及18个不同图片](<prompt-history/README.md>)、[机器可读事件](<prompt-history/events.json>)。用户记录覆盖截至本次补齐请求 seq3339；系统、代理消息不冒充用户Prompt，后续没有新增用户纠错则无需虚构补写。

[本轮材料独立复核与处理](<../review/材料复核.md>)记录了并发审查发现的旧核验矛盾、补齐项和最终处理。

## 5. 完成范围

B代码、报告、提示、日志、截图、补丁与哈希核验均在本交付目录。最终提交SHA尚未产生，因为本轮明确禁止commit/push；实际提交与小组整合仍由本人/组员执行。不代表Buddy或全组任务全部完成。

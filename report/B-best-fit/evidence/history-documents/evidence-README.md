# B 部分验证证据索引

**结论：编译成功；普通运行没有取得自检成功；真实 make grade 失败。没有 20/20、两个 PASS 或成功截图。**

## 1. 主验证批次：2026-10-24 01:53:10 +08:00 开始

- [执行会话](<verification-session.log>)记录 clean、build、普通运行、正式 grade 的命令、输出和退出码。该记录结尾只保存了 END 字样，结束时间没有完整写入；不补造时间。
- [环境版本](<environment.log>)；[构建日志](<build.log>)及[构建退出码 0](<build.exitcode>)。
- [普通运行](<qemu.log>)及[退出码 124](<qemu.exitcode>)：15 秒超时，OpenSBI 的 Next Address=0，没有内核自检成功。
- [正式评分输出](<grade.log>)及[退出码 2](<grade.exitcode>)：两项 WRONG，Total Score: 0/20。
- [同次评分串口](<grade-serial.log>)：仅 OpenSBI，Next Address=0，两个 PASS 均不存在。这条是主批次观察，不转嫁到其他批次。
- [ELF 程序头](<kernel-program-headers.log>)、[镜像大小](<image-stat.log>)、[构建产物哈希](<build-artifacts.sha256>)用于排查，不作为通过证明。

主脚本有意继续采集失败后的下一阶段，脚本自身退出 0 不等于评分退出 0；以每个 .exitcode 文件为准。正式 grade 是未改评分脚本的真实目标，不是自行注入运行。

## 2. 终端复跑尝试一：terminal-run

[会话日志](<terminal-run/terminal-session.log>)、[build](<terminal-run/build.log>)、[QEMU](<terminal-run/qemu.log>)、[grade](<terminal-run/grade.log>)、[同次 grade 串口](<terminal-run/grade-serial.log>)及对应退出码保留。build=0、QEMU=124、grade=2，总分仍为 0/20。终端等待输入遇到非阻塞 stdin 错误，未在此批次取得完整评分截图；不把后一次截图配作此批次截图。

## 3. 真实桌面截图批次：terminal-capture

使用实际 Windows 控制台启动 WSL；script 提供伪终端但不改变 Makefile 或评分脚本，退出码取 PIPESTATUS[0]。截图由屏幕抓取工具取得，没有用文字生成图片，没有编辑或合成结果。

| 阶段 | 实际运行时间（+08:00） | 退出码 | 证据 |
|---|---|---:|---|
| build | 见该阶段命令与输出 | 0 | [命令](<terminal-capture/build.command.log>)、[日志](<terminal-capture/build.log>)、[退出码](<terminal-capture/build.exitcode>) |
| 普通 QEMU | 02:04:01–02:04:16 | 124 | [命令](<terminal-capture/qemu.command.log>)、[日志](<terminal-capture/qemu.log>)、[退出码](<terminal-capture/qemu.exitcode>)、[真实截图](<terminal-capture/qemu-actual.png>) |
| 正式 make grade | 02:06:26–02:06:28 | 2 | [命令](<terminal-capture/grade.command.log>)、[日志](<terminal-capture/grade.log>)、[退出码](<terminal-capture/grade.exitcode>)、[真实截图](<terminal-capture/grade-actual.png>) |

截图批次的 [grade 串口](<terminal-capture/grade-serial.log>)只写到 Domain0 Region00 的半行（共 28 行），没有 Next Address 行，更没有 PASS。因此不能声称本次截图所对应的 grade 串口也明确显示 Next Address=0；该数值来自主批次和普通 QEMU 的日志。保留原始截断，不拼接其他批次填补。

grade 截图中还显示 WSL 网络回退和 tar 轻微时间戳警告，均原样保留；没有证据把它们认定为评分失败的全部根因。截图中错误消息引用的 missing '...PASS' 不是 PASS 成功输出。

![普通 QEMU 真实超时画面](<terminal-capture/qemu-actual.png>)

![正式 make grade 真实 0/20 画面](<terminal-capture/grade-actual.png>)

## 4. 基线、差异和提示

[基础归档核验](<baseline-verification.json>)、[受测源码范围审计](<scope-audit.json>)、[代码补丁](<B-best-fit.patch>)、[diff stat](<diff-stat.log>)、[提示记录索引](<../prompts.md>)。只允许两个代码文件变化，不包括启动链和评分配置。所有新增材料的最终清单及哈希在交接时复核。

## 5. 仍缺的验收证据

普通启动的 check_alloc_page() succeeded!、同一次正式评分的退出码 0 / 两项 OK / Total Score: 20/20 / 串口两个 PASS，及其真实成功截图，均未取得。当前文件只是可复查的失败记录。不得修改日志、评分脚本或启动配置来拼出通过结果。

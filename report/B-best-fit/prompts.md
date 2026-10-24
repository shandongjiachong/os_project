# B 部分实际 AI 协作记录

姓名：王泊凯；学号：2412224。以下区分真实发送记录、预写分工提示词和事后说明，不把它们混为同一回事。

## 1. 首次实际用户 Prompt

[完整原文](<evidence/initial-user-prompt.txt>)从本会话事件 seq 9 提取，要求读取分工、代码和指导书，并使用 DeepSeek v4.1 Flash max。原分工中的预写提示不是每一轮实际对话的替代品。

## 2. 最初实现任务及实际执行者

[Best-Fit 任务提示](<evidence/historical-best-fit-dispatch.txt>)来自实际工具调用 seq 87，包含全部接口、全扫描最小适配、切割、合并和禁止修改测试的要求。文件保留历史代码字符串中的转义反引号。

该次请求的路由是 deepseek/deepseek-v3-250324，返回错误：


> child LLM route "deepseek/deepseek-v3-250324" is not allowed for this Session

这既不是用户指定的 Flash 路由，也没有成功执行。不能写成“DeepSeek Flash 首次生成并通过验证”。历史源码由主助手在后续工具调用（seq 108；管理器选择 seq 122）写入。本次没有重新虚构一次算法生成过程。

## 3. 后续实际约束与最终工作 Prompt

- [身份与分支要求原文](<evidence/identity-and-branch-prompt.txt>)：明确王泊凯、2412224、无需个人分支。其当时的“只给命令”约束后来由下一条执行要求取代。
- [最终执行 Prompt 完整原文](<evidence/current-user-request.txt>)：只做 B；隔离目录；保留队友与测试；真实编译、普通运行和正式评分；保存真实截图；不得 commit/push；失败必须如实记录。本次按这条执行。
- [只读审查 Prompt 完整原文](<evidence/review-prompt.txt>)：发送给实际成功运行的 deepseek-official/deepseek-flash、reasoning=max 子智能体，要求检查最小适配、顺序、计数、合并和保护范围，不构建、不改文件。会话 ID：0395ee64-0519-4b7f-8978-eed5cb1a9476。

## 4. 审查输出与人工复核

[原始审查结果](<review/deepseek-flash-max-raw.md>)和[主审复核](<review/主审复核.md>)同时保留。采纳静态不变量检查；当时不采纳“0/20 是假阴性”“换版本即可通过”等未经本副本运行验证的判断；后来 QEMU 4.1.1 的真实复跑补足通过证据，旧批次仍如实记为失败。主审还据实际逐页清理循环，把释放总复杂度明确为 O(n+k)，而非仅记链表搜索的 O(k)。

本次代码迁移保留原有 Best-Fit 算法，只填写 B 身份并在远程基线上改变管理器选择。没有已证实的 Best-Fit 算法错误需要编造修复记录。历史有关 PGP 文件类型、objcopy 偏移及镜像大小的错误判断已在[实现说明](<说明.md>)明确纠正，未混入本次源码。

## 5. 已恢复的逐轮原文与图片反馈

本轮通过原始事件读取接口，连续扫描会话 seq 1–3520，并按 source.kind=user 筛选，恢复截至本次补齐请求 seq 3339 的 48 条真实用户消息。包含图像反馈 19 次引用，对应 18 张不同附件；每张按 SHA-256 原样保存，不改图、不把截图 OCR 结果冒充用户文本。完整文本与图像链接见[逐轮原文](<evidence/prompt-history/README.md>)，机器可读数据见[events.json](<evidence/prompt-history/events.json>)。该范围的用户消息已完整恢复；这不是助手全部回答或所有工具调用的全量聊天备份。

原分工中的预写消息保存在[分工原文](<evidence/B-assignment-original.txt>)。这些是建议发送的模板，不等于每一条都实际发送过。原文中的个人分支建议后来被本人明确要求的 Lab2 目标分支及禁止本轮 commit/push 所取代。

## 6. 环境纠错链与本轮收敛

- seq 2952：将修复范围限定为 Best-Fit 与管理器选择，禁止更改启动链和测试。
- seq 3104：明确拒绝启动兼容性修改，要求改走课程认可环境。
- seq 3155：同意独立安装 QEMU 4.1.1；编译器版本若不确定先确认。
- 工具交互确认：本人选择“允许沿用现有工具链”，故保留 GCC 15.1.0；不将它标注为 SiFive 官方发行。
- seq 3270：补充课程本地环境说明；该文没有指定工具链发行版本。
- seq 3339：要求补齐 B 的全部材料。

普通自检与原版评分已在 QEMU 4.1.1 下实测通过，原失败记录作为历史保留。此前“不保证换版本即可通过”是当时正确的证据边界，后来通过真实运行补足了结论，并非事后删除失败过程。详见[实现报告](<说明.md>)与[验证证据](<evidence/README.md>)。

## 7. 可复用的最终 spec（事后整理，非历史原文）

该版本仅方便小组复现或讨论，不能冒充第一次发送的 Prompt：

```text
在既有 Lab2 框架中实现 best_fit_init、best_fit_init_memmap、best_fit_alloc_pages、best_fit_free_pages、best_fit_nr_free_pages，并注册 best_fit_pmm_manager。遍历全部候选，选择长度至少为 n 的最小块，大小相同按地址序保留首候选；切割余块保持链表有序。释放先有序插入，再按物理邻接关系合并前后块。nr_free 只因真实分配、释放改变，分配失败不改状态。
仅修改 Best-Fit 实现与 init_pmm_manager 的选择语句；保留 page_init、框架测试、链表库、结构体、启动及评分配置。填写学号 2412224。
使用独立 QEMU4.1.1 与经确认的交叉工具链。执行 clean、build、原 make qemu、自检以及原 make grade；保留实际命令、退出码、完整日志、同次串口两个PASS和真实截图。不能用评分Test2指针检查替代普通best_fit_check。
发生启动层问题先核实环境，不越界改启动文件或评分规则；不commit/push。报告区分实测、静态推演和未覆盖场景。
```

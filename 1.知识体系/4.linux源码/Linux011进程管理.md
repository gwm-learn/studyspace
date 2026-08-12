# Linux011进程管理

Linux 0.11 的进程管理代码集中在 `kernel/` 目录：fork.c（进程创建）、sched.c（调度与时钟）、exit.c（进程退出）、signal.c（信号）、system_call.s（系统调用入口）。每个进程对应一个 `task_struct`，所有进程的task_struct在静态数组 `task[NR_TASKS]` 中，NR_TASKS为64。

## 进程的数据结构

```c
struct task_struct {
    long state;          // 进程状态：运行/可中断/不可中断/停止/僵死
    long counter;        // 剩余运行时间片，调度依据
    long priority;       // 优先级，fork时从父进程继承
    long signal;         // 收到的信号位图
    ...                  // 文件描述符、信号处理、父子进程指针等
    struct tss_struct tss;   // 硬件任务状态段
};
```

进程0（init_task）是编译期静态定义的，是整个进程树的根。它没有父进程，task[0]永远指向它。

## 进程状态

| 状态 | 宏 | 含义 |
| :--- | :--- | :--- |
| 运行 | TASK_RUNNING | 正在运行或可被调度 |
| 可中断睡眠 | TASK_INTERRUPTIBLE | 可被信号唤醒 |
| 不可中断睡眠 | TASK_UNINTERRUPTIBLE | 只能被wake_up唤醒 |
| 停止 | TASK_STOPPED | 收到SIGSTOP暂停 |
| 僵死 | TASK_ZOMBIE | 已退出，等待父进程回收 |

## fork 创建进程

fork 是唯一的进程创建入口（内核启动时的init是特例），整体调用链为：用户态 `fork()` → int 0x80 → `system_call` → `sys_fork` → `find_empty_process` → `copy_process`。

1. `find_empty_process`：递增 `last_pid`，在task[]中找一个空槽，pid保证不与现有进程重复。
2. `copy_process`：`get_free_page()` 分配一页内存作为新进程的task_struct和内核栈。
3. 复制父进程task_struct中的大部分字段，只改写pid、ppid、创建时间等。
4. `copy_mem`：复制页表。Linux 0.11没有写时复制（COW），fork时直接把父进程的页表复制一份，父子进程映射同一物理页，直到exec时进程1的内存被替换。
5. 在task[nr]填入新进程，新进程状态设为就绪，等待调度。

fork 返回两次：父进程得到子进程的pid，子进程返回0。这是通过copy_process最后压栈的eax值实现的。

## exec 执行新程序

exec 系列系统调用最终调用 `do_execve`，调用链为：用户态 `execve` → int 0x80 → `sys_execve` → `do_execve`（kernel/exec.c）。

1. 解析可执行文件头（a.out格式），读取魔数、代码段/数据段长度。
2. 检查权限，读取文件到内核缓冲。
3. 释放原进程的代码段和数据段页表，按新程序的代码段、数据段大小重新分配物理页并加载内容。
4. 重置栈和堆，设置新的CS/EIP指向程序入口。
5. 保留文件描述符表，替换进程映像，进程继续以新程序身份运行。

## 进程0与进程1

| 进程 | pid | 创建方式 | 职责 |
| :--- | :--- | :--- | :--- |
| 进程0 | 0 | 编译期静态定义 | 空闲进程，main初始化后进入`for(;;) pause()` |
| 进程1 | 1 | main中`fork()` | init进程，挂载根文件系统，启动/bin/sh |
| 进程2 | 2 | init中`fork()` | 尝试加载/etc/rc配置并执行/bin/sh |

main.c 中 `move_to_user_mode()` 用伪造栈帧+iret把进程0从内核态切到用户态；随后 `fork()` 产生进程1执行init()，进程1再 `fork()` 产生进程2。init() 中调用 `setup()` 挂载根文件系统，然后设置stdin/stdout/stderr指向/dev/tty0。

## 调度算法

Linux 0.11 采用基于时间片的轮转调度，核心是 `schedule()`（kernel/sched.c）。

1. 先处理任务：把超时（alarm）且可中断睡眠的进程唤醒，处理SIGCHLD信号。
2. 遍历task[]，选出 `state==TASK_RUNNING && counter` 最大的进程作为next（counter越小越没机会）。
3. 若所有运行进程的counter都为0，则把所有进程（含睡眠的）的counter按 `counter>>1 + priority` 重新填充，相当于刷新时间片。
4. `switch_to(next)` 切换TSS，CPU自动保存/恢复寄存器上下文。

时钟中断每10ms触发一次 `do_timer`：`jiffies++`，当前进程counter减1，减到0就调用schedule。这样每个进程公平分享CPU时间，counter相当于剩余时间片。

## 进程切换

`switch_to` 用任务门跳转（ljmp到新进程的tss选择子），利用CPU硬件任务切换机制：CPU自动把当前寄存器保存到旧TSS，再从新TSS恢复。因此Linux 0.11的上下文切换不用手工保存寄存器，切换代码非常短。

## 系统调用入口

所有系统调用都走同一入口 `system_call`（system_call.s）：

1. 用户程序触发 `int 0x80`，CPU切换到内核栈。
2. 保存现场，检查 `%eax`（调用号）是否合法。
3. 根据调用号跳转到 sys_call_table 中对应的C函数。
4. 返回前用 `ret_from_sys_call` 检查是否需要重新调度（有信号或counter耗尽）。

进程0在 `main` 里用 `move_to_user_mode` 切换到用户态，之后才fork出init，因此进程0实际是唯一从内核态"变成"用户态的进程，其余进程全部从fork产生。

## 关键点总结

1. fork复制整个地址空间（无COW），exec才替换进程映像，这是0.11与现代Linux最大的差异。
2. 调度只看counter，简单的时间片轮转，没有优先级继承和O(1)运行队列。
3. 进程切换借助硬件TSS，一条ljmp完成保存和恢复。
4. 时钟中断驱动调度，jiffies是全局心跳。
5. 进程0/1/2分别扮演空闲、初始化、shell三兄弟，是系统后续所有进程的祖先。

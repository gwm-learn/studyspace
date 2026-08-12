# Linux011文件系统

Linux 0.11 使用MINIX 1.0文件系统，源码位于 `Linux011/fs/`。整个文件系统由super块、inode表、位图和数据区组成，内核通过高速缓冲（buffer cache）访问块设备，向上提供VFS风格的统一接口。

## 磁盘布局

MINIX 1.0 文件系统在磁盘上按以下顺序排列：

| 区域 | 内容 |
| :--- | :--- |
| 引导块 | 第0块，存放引导程序 |
| 超级块 | 第1块，记录文件系统元信息 |
| i节点位图 | 标记哪些i节点已使用 |
| i节点表 | 存放所有i节点，0.11支持1~32个块 |
| 逻辑块位图 | 标记哪些数据块已使用 |
| 数据区 | 存放文件内容和目录内容 |

超级块包含魔数（0x137F）、i节点数、逻辑块数、每个块大小、每个i节点的数据块数等。`super.c` 中的 `read_super` 读取并校验超级块，魔数不匹配就报"VFS: Mounted file system doesn't match"。

## i节点与目录项

i节点（inode）是文件系统的最小管理单元，记录文件的模式、属主、大小、时间以及数据块的位置（0.11是10个直接块+1个一级间接块+1个二级间接块）。

```
struct m_inode {
    unsigned short i_mode;    // 文件类型与权限
    unsigned short i_uid, i_gid;
    unsigned short i_nlinks;  // 硬链接数
    unsigned short i_zone[9]; // 0~6直接块，7一级间接，8二级间接
    ...
};
```

目录就是内容为目录项的普通文件，每个目录项是16字节：2字节i节点号+14字节文件名。目录项分散在数据块中，`namei.c` 的查找逻辑就是顺序扫描目录数据块，逐项比较文件名。

## VFS层

文件系统向内核提供统一接口，核心是两个操作表：

| 操作表 | 定义 | 内容 |
| :--- | :--- | :--- |
| file_operations | fs/file_table.c | read、write、lseek、open、release |
| inode_operations | fs/inode.c | lookup、create、mkdir、unlink等 |

具体到MINIX文件系统，read/write等实现分布在file_dev.c（普通文件）、block_dev.c（块设备）、char_dev.c（字符设备）、pipe.c（管道）中。内核用"文件类型+操作表"分发：同一套sys_open/sys_read代码可以同时处理普通文件、设备文件和管道。

## 挂载根文件系统

根文件系统在启动早期由 `mount_root`（fs/super.c）完成挂载，调用链为 main.c 的 init() → `setup()` → `mount_root`：

1. `rd_load`/`hd_init` 等先就绪块设备（硬盘或RAM盘）。
2. 读超级块并校验魔数，把超级块挂到 `super_block[8]` 表中。
3. 把根i节点（1号i节点）读入内存，`current->root` 和 `current->pwd` 指向它。
4. 至此路径解析有了起点，/bin/sh 才能通过相对/绝对路径找到。

## open 调用链

用户态 `open()` → int 0x80 → `sys_open`（fs/open.c）：

1. `open_namei`（namei.c）把路径名逐级解析：从根或当前目录出发，用 `dir_namei` 定位父目录，再 `lookup` 在目录中查找文件名，最终得到目标i节点。
2. 按flag检查权限：O_RDONLY/O_WRONLY/O_RDWR 与i节点mode比对，O_CREAT时创建新文件并分配i节点。
3. `get_empty_filp` 从 `file_table[NR_FILE]`（64项）取一个空闲file结构，填入i节点指针、读写模式和操作表。
4. `current->filp[fd] = filp` 把fd绑定到file结构，返回fd。

`sys_creat` 是 `sys_open(pathname, O_CREAT|O_TRUNC, mode)` 的包装。

## read/write 调用链

用户态 `read()` → int 0x80 → `sys_read`（fs/read_write.c）：

1. 用fd从 `current->filp[fd]` 取file结构，取文件偏移f_pos。
2. 按file的操作表分发：普通文件走 `read_file`（file_dev.c），管道走 `read_pipe`（pipe.c），块设备走 `block_read`，字符设备走 `rw_char`。
3. 普通文件：把字节偏移换算成逻辑块号，逐块 `bread`（buffer.c）读入高速缓冲，拷到用户缓冲区，更新f_pos。

write 对称：`sys_write` → `write_file` → 从用户缓冲区拷入缓冲块，块通过 `ll_rw_block` 异步写回磁盘（真正落盘靠bdflush或同步）。所有块IO都经过buffer cache，这是块设备驱动之上的统一层。

## 高速缓冲 buffer.c

buffer cache 是文件系统与块设备驱动之间的桥梁：

| 函数 | 作用 |
| :--- | :--- |
| getblk | 按块号查找缓冲块，不存在则分配 |
| bread | getblk后读盘，返回有效数据 |
| brelse | 释放缓冲块引用 |
| ll_rw_block | 提交底层读写请求（块设备驱动） |

buffer_init 在main中由 `buffer_init(buffer_memory_end)` 初始化，缓冲池大小由内存总量决定。块设备驱动的hd.c/floppy.c通过 `request[32]` 请求队列处理ll_rw_block提交的IO。

## 设备文件的读写

设备也是文件，靠i节点的i_mode标识类型（块设备/字符设备），设备号存在i_zone[0]中。

| 设备类型 | 读函数 | 写函数 | 实现 |
| :--- | :--- | :--- | :--- |
| 普通文件 | read_file | write_file | fs/file_dev.c，走buffer cache |
| 块设备 | block_read | block_write | fs/block_dev.c，按设备号找驱动 |
| 字符设备 | rw_char | rw_char | fs/char_dev.c，按设备号分发到tty等 |
| 管道 | read_pipe | write_pipe | fs/pipe.c，无盘、纯内存缓冲 |

`sys_read`/`sys_write` 在确定file结构后，根据i节点的类型选择对应的函数。管道文件没有对应的磁盘块，写入的数据放在i节点的直接块（实际是内存页）中，读空则阻塞、写满则阻塞，是进程间通信的基础。

## 关键点总结

1. MINIX 1.0是简单的直链式文件系统，i节点用直接+间接块寻址，最大文件约64MB。
2. VFS层的核心是file_operations和inode_operations两张操作表，让系统调用与具体文件系统解耦。
3. 路径解析（namei.c）是全系统最复杂的逻辑之一，open/stat/chdir都依赖它。
4. 所有块读写都经过buffer cache，与Linux 0.11的"一切皆文件"理念相配合，设备也被当作文件挂载。
5. 根文件系统挂载发生在init进程中，先于/bin/sh的启动。

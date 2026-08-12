# AGENTS.md — studyspace

> 个人学习与工作的知识框架。This is a **knowledge archive**, not a unified software project.  
> Primary language: Chinese. Top-level directories are topic areas, not packages.

## What every agent must know

- **Do not attempt a repo-wide build.** There is no root build system, no root `Makefile`, no root `CMakeLists.txt`, and no root `.gitignore`.
- Every subdirectory with source code is an **independent project** with its own build tooling. Check inside each before assuming anything.
- Many directories contain **vendored third-party source** kept for study. Do not modify these unless the task is explicitly about them.

## Vendored source trees (read-only for study)

| Path | What it is |
|------|------------|
| `3.协议应用/2.snmp/src/` | Full net-snmp 5.9.4 source (autotools) — 2258 tracked files |
| `1.知识体系/4.linux源码/Linux011/` | Full Linux 0.11 kernel source (hand-written Makefiles) |
| `1.知识体系/7.openwrt/3.网络业务/2.关键组件服务/1.网络管理/netifd/` | Full netifd source (CMake) |
| `1.知识体系/3.linux驱动/led/leds/` | Linux kernel LED driver sources (Kbuild) |

**Vendored trees have their own CI** (net-snmp has `.github/workflows/`), their own `.gitignore`, and their own build scripts. These are NOT repo-level config.

## The user's own code projects

Located under `2.编程技能/1.C语言/`. Each is a small, self-contained C practice project:

| Directory | Build |
|-----------|-------|
| `fw_params/` | `make` (has nested `src/Makefile`) |
| `produce/` | `make` (has nested `src/Makefile`) |
| `mobile-mngr/` | `make` |
| `ipc/Template/7-socket/socket_server_client/` | `make` |
| `ipc/TEST/` | CMake (`mkdir build && cd build && cmake .. && make`) |
| `net/` | No build system — standalone `.c` files |

## Custom kernel drivers

| Path | Build |
|------|-------|
| `1.知识体系/3.linux驱动/gpio/gpio_customize/` | `make M=/path/to/gpio_customize modules` from kernel source root |

## Existing AGENTS.md files (sub-project level)

- `1.知识体系/3.linux驱动/gpio/AGENTS.md` — GPIO driver build, test, kernel coding style
- `1.知识体系/7.openwrt/3.网络业务/2.关键组件服务/1.网络管理/netifd/AGENTS.md` — netifd build (CMake), style, architecture

These contain detailed per-project guidance. Read them before working in those directories.

## Repository conventions

- **Chinese is the primary language** for notes, diagrams, and documentation. Code comments may mix Chinese and English.
- **File naming**: `.md` for notes, `.drawio` + `.png` for architecture diagrams, PDFs for reference books/specs.
- **`.gitkeep`** is used in 13 empty directories to preserve structure. Do not remove them.
- **No lint/format config at root.** A `.clang-format` exists at `4.其他技能/2.shell/文件操作相关/.clang-format` if you need one.

## Git

- Remote: `git@github.com:gwm-learn/studyspace.git`
- Branch: `master`
- No pre-commit hooks, no CI at repo root
- `.omo/` is untracked (agent session data) — ignore it

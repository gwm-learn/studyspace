# AGENTS.md - netifd Codebase Guide for Agentic Coding

## Project Overview
**netifd** is the OpenWrt network interface daemon written in C. It manages network interfaces, protocols, and wireless configuration. The codebase uses CMake for building and integrates with OpenWrt's libubox, libubus, libuci, and json-c libraries.

## Build System

### Primary Build Tool
- **CMake** (version 2.6 or higher)
- No Makefile, Autotools, or Meson

### Standard Build Commands
```bash
# Create a build directory (out‑of‑source build recommended)
mkdir build
cd build

# Configure with default options
cmake ..

# Build the netifd executable
make

# Install (requires root)
sudo make install
```

### Build Options
Options can be passed to `cmake` via `-D` flags:

| Option | Description | Default |
|--------|-------------|---------|
| `DEBUG` | Enable debug symbols and logging (`-DDEBUG -g3`) | OFF |
| `NO_OPTIMIZE` | Disable optimization (`-O0`) when `DEBUG=ON` | OFF |
| `DUMMY_MODE` | Build for dummy (non‑Linux) environment | OFF (auto‑detected) |

Example debug build:
```bash
cmake -DDEBUG=ON -DNO_OPTIMIZE=ON ..
```

### Custom Target
- `ethtool-modes-h`: generates `ethtool-modes.h` via `make_ethtool_modes_h.sh`
- This target is automatically built as a dependency of `netifd`.

### Build Output
- Executable: `netifd` (installed to `/sbin` by default)
- No static or shared libraries.

## Test Commands

### Unit / Integration Tests
**No unit test suite or test runner is present in the repository.**  
The codebase does not contain a `tests/` directory, `ctest` configurations, or any `*_test.c` files.

### Testing Approach
- Testing is likely performed via system‑level integration (OpenWrt's build‑system tests).
- For local validation, run the built daemon with the `DUMMY_MODE` flag.

### Running a Single Test
Not applicable. If you add tests later, follow the OpenWrt test framework conventions.

## Code Style Guidelines

### Indentation
- **Tabs** for indentation (not spaces).  
- Tab width is 8 characters (typical for OpenWrt C projects).

### Brace Style (K&R variant)
- Function braces on the **next line**:
  ```c
  static void
  netifd_process_cb(struct uloop_process *proc, int ret)
  {
      // ...
  }
  ```
- Control‑statement braces on the **same line**:
  ```c
  if (!len) {
      break;
  } else {
      // ...
  }
  ```
- Single‑statement blocks may omit braces:
  ```c
  if (!len)
      break;
  ```

### Naming Conventions
- **snake_case** for variables, functions, and file names.
- **Prefixes** denote module/scope:
  - `netifd_` – core daemon functions
  - `vlist_` – virtual‑list utilities
  - `interface_`, `device_`, `proto_` – module‑specific
- **Macros** and constants: `UPPER_SNAKE_CASE`
- **Structs**: `struct netifd_process`
- **Typedefs**: not heavily used; prefer explicit `struct` references.

### Includes Order
1. System headers (`<stdio.h>`, `<stdlib.h>`, etc.)
2. Library headers (`<libubox/uloop.h>`, `<libubus.h>`, etc.)
3. Project local headers (`"netifd.h"`, `"utils.h"`, etc.)

Group includes with a blank line between groups. Example from `main.c`:
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>

#include "netifd.h"
#include "ubus.h"
#include "config.h"
#include "system.h"
#include "interface.h"
#include "wireless.h"
#include "proto.h"
#include "extdev.h"
```

### Error Handling
- **Error returns**: functions return `-1` (or `NULL`) on failure, `0` on success.
- **goto cleanup**: the preferred pattern for resource cleanup:
  ```c
  int example(void)
  {
      if (setup() < 0)
          goto error;
      // ...
      return 0;
  error:
      cleanup();
      return -1;
  }
  ```
- **Error logging**: use `netifd_log_message()` with severity levels (`L_DEBUG`, `L_INFO`, `L_NOTICE`, `L_WARNING`, `L_CRIT`).

### Comments
- **Block comments**: `/* ... */` for file headers and multi‑line explanations.
- **Single‑line comments**: `//` for in‑line notes (used for TODOs).
- **TODO/FIXME**: mark with `// TODO: ...` or `/* TODO: ... */`.
- **Doxygen**: not used; keep comments concise and technical.

### Line Length
No strict limit; typical lines are under 100 characters. Break long lines at logical points.

### Macro Usage
- Use `#define` for constants and simple inline functions.
- Wrap multi‑statement macros in `do { ... } while (0)`.
- Prefer inline functions over complex macros when possible.

### Type Safety
- Use standard C99 types (`uint32_t`, `size_t`, etc.).
- Avoid `void*` unless necessary for generic containers.
- Explicit casts only when required (e.g., pointer to integer).

## Project Structure

```
.
├── CMakeLists.txt          # Main build configuration
├── .gitignore             # Build artifacts, temporary files
├── main.c                 # Entry point, daemon initialization
├── netifd.h               # Core definitions and includes
├── *.c / *.h              # Module sources (interface, device, proto, etc.)
├── config/
│   ├── board.json        # Board‑specific configuration
│   ├── network           # Network configuration examples
│   └── wireless          # Wireless configuration examples
├── examples/
│   ├── hotplug-cmd       # Example hotplug script
│   └── proto/            # Example protocol scripts (ppp.sh, pptp.sh)
├── scripts/
│   ├── netifd-wireless.sh # Wireless backend script
│   ├── netifd-proto.sh    # Protocol backend script
│   └── utils.sh           # Common shell utilities
└── make_ethtool_modes_h.sh # Helper for ethtool mode generation
```

### Key Files
- `main.c`: daemon entry point, signal handling, process management
- `interface.c`, `device.c`, `proto.c`: core business logic
- `system‑linux.c`: Linux‑specific implementation (or `system‑dummy.c` in dummy mode)
- `ubus.c`: ubus RPC interface
- `utils.c`: generic data‑structure helpers (virtual lists, string utilities)

### Dependencies
- **libubox** (uloop, ustream, utils)
- **libubus**
- **libuci**
- **json‑c** (or libjson)
- **libnl‑3** (Linux‑only, for netlink communication)

## Agent‑Specific Notes

### How to Build for Debugging
1. Set `DEBUG=ON` and `NO_OPTIMIZE=ON` to get full symbols and disable optimizations.
2. Run `gdb ./netifd` from the build directory.
3. Use `DUMMY_MODE=ON` to test without requiring Linux netlink.

### How to Add a New Source File
1. Add the `.c` file to the `SOURCES` list in `CMakeLists.txt`.
2. Create a corresponding header file (if needed) and add to `#include` groups.
3. Follow existing naming and style patterns.

### How to Run a Single Test (When Tests Exist)
Not applicable. If you introduce a test suite, integrate with `ctest` and document the command pattern here.

### Version Control
- Branching model: follows OpenWrt’s upstream git workflow.
- Commit messages: short summary line, blank line, detailed description.
- Pre‑commit hooks: none configured.

### Linting and Formatting
- No `.clang‑format`, `.astylerc`, or `.editorconfig` present.
- Rely on the existing codebase as the style reference.

### Cursor / Copilot Rules
No `.cursor/rules/`, `.cursorrules`, or `.github/copilot‑instructions.md` files found. Agents should adhere to the conventions described in this document.

---

*This AGENTS.md was generated based on analysis of the netifd repository on 2026‑03‑07. Update it as the codebase evolves.*
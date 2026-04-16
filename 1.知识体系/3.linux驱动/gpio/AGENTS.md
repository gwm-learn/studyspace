# AGENTS.md - GPIO Linux Driver Project

This document provides essential information for agentic coding agents working on this Linux GPIO driver project. It covers build commands, testing procedures, and code style guidelines.

## Project Overview

This is a Linux kernel GPIO driver module that provides sysfs interface for GPIO pins, similar to the LED subsystem. The driver creates `/sys/class/gpio_customize/` entries for each configured GPIO, allowing user-space control via `value` files.

**File Structure:**
```
gpio/
└── gpio_customize/
    ├── gpio_customize.c      # Driver core implementation
    ├── Makefile              # Kbuild configuration
    └── README.md             # Project documentation
```

## Build Commands

### Prerequisites
- Linux kernel source tree (same version as target system)
- Kernel headers installed
- Build tools (gcc, make, etc.)

### Building as a Module
Navigate to the Linux kernel source root directory:

```bash
# From kernel source root
make M=/home/gwm/code/studyspace/1.知识体系/3.linux驱动/gpio/gpio_customize modules
```

Or if the driver is placed in the kernel tree at `drivers/gpio_customize/`:

```bash
make M=drivers/gpio_customize modules
```

### Output
The build produces `gpio_customize.ko` in the same directory as the source files.

### Clean Build
```bash
make M=/path/to/gpio_customize clean
```

### Module Installation
```bash
sudo insmod gpio_customize.ko
```

### Module Removal
```bash
sudo rmmod gpio_customize
```

### Module Information
```bash
modinfo gpio_customize.ko
```

## Testing Commands

### Current Test Status
This project does **not** have automated tests. Testing is manual through:
1. Loading the module and verifying it appears in `/sys/class/gpio_customize/`
2. Reading/writing GPIO values via sysfs
3. Checking kernel logs (`dmesg`) for errors

### Manual Testing Commands
```bash
# Check if module loaded
lsmod | grep gpio_customize

# Check sysfs interface
ls /sys/class/gpio_customize/

# Read GPIO value
cat /sys/class/gpio_customize/<device-name>/value

# Write GPIO value
echo 1 > /sys/class/gpio_customize/<device-name>/value
echo 0 > /sys/class/gpio_customize/<device-name>/value

# Monitor kernel logs
dmesg -w
```

### Test Recommendations
Consider adding:
- Kernel module unit tests using KUnit
- Device tree test cases
- Integration tests with real/simulated GPIO

## Code Style Guidelines

This project follows **Linux kernel coding style** since it's a kernel module. All contributions must adhere to these conventions.

### General Principles
- Follow the Linux kernel coding style documented in `Documentation/process/coding-style.rst`
- Use kernel-specific APIs and patterns
- Prioritize stability and compatibility over features

### Indentation
- **Tabs, not spaces** for indentation
- Tab width: 8 characters
- Continuation lines indent 2 tabs (16 spaces) from function name

### Braces Placement
- Functions: Opening brace on next line
- Control statements (if, for, while, switch): Opening brace on same line
- Non-function blocks: Opening brace on same line

**Examples:**
```c
static int gpio_customize_probe(struct platform_device *pdev)
{
    /* function body */
}

if (condition) {
    /* if body */
}
```

### Naming Conventions
- **Functions**: `lowercase_with_underscores` (prefix with `gpio_customize_` for driver-specific functions)
- **Variables**: `lowercase_with_underscores`
- **Constants**: `UPPERCASE_WITH_UNDERSCORES` (prefix with `GPIO_CUSTOMIZE_` for driver-specific constants)
- **Types**: `struct gpio_customize_data`, `enum gpio_customize_default_state`
- **Macros**: `UPPERCASE_WITH_UNDERSCORES` (except for function-like macros which may use lowercase)

### Header Includes Order
1. Main module header (`#include <linux/module.h>`)
2. Kernel headers (`#include <linux/kernel.h>`, `#include <linux/err.h>`, etc.)
3. Subsystem headers (`#include <linux/gpio/consumer.h>`, `#include <linux/platform_device.h>`)
4. Driver-specific headers (none in this project)
5. Local headers (none in this project)

**Example from existing code:**
```c
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
```

### Error Handling
- Return negative `errno` values (e.g., `-ENODEV`, `-EINVAL`, `-ENOMEM`)
- Use `IS_ERR()` and `PTR_ERR()` for pointer error checking
- Propagate errors up the call chain
- Clean up allocated resources on error paths

**Example:**
```c
if (IS_ERR(data->gpiod)) {
    dev_err(parent, "Failed to get GPIO for %s: %ld\n",
            name, PTR_ERR(data->gpiod));
    return PTR_ERR(data->gpiod);
}
```

### Comment Style
- Single-line comments: `//` for brief comments
- Multi-line comments: `/* ... */` for longer explanations
- Function documentation: Brief comment above function
- Kernel-doc format: Not used in this project, but acceptable

**Example:**
```c
/*
 * GPIO Customize driver
 *
 * Copyright (C) 2025
 * Author: AI Assistant
 *
 * This driver creates sysfs interface for GPIO pins similar to LED subsystem.
 */
```

### Typing and Type Usage
- Use kernel-specific types: `u8`, `u16`, `u32`, `u64`, `size_t`, `ssize_t`
- Use `__`-prefixed types only when necessary for architecture-specific code
- Avoid standard C types (`int`, `long`) when kernel types are more appropriate

### Formatting Preferences
- Space after keywords: `if (`, `for (`, `while (`, `switch (`
- No space after function names: `function_name(args)`
- Pointer declarations: `struct device *dev` (asterisk adjacent to variable name)
- Line length: Try to stay under 80 columns, but 100 is acceptable

### Device Tree Bindings
- Compatible string: `"gpio-customize"`
- Node naming: `gpio-customize#` (e.g., `gpio-customize0`, `gpio-customize1`)
- Properties: `gpios`, `label` (optional), `default-state` (optional)
- Follow existing `gpio-leds` binding patterns

## Configuration Files

### Existing Configuration
- **Makefile**: Simple Kbuild configuration with `obj-y += gpio_customize.o`
- **No other configuration files** present (.clang-format, .editorconfig, etc.)

### Recommended Additions
Consider adding:
- `.clang-format` with Linux kernel settings
- `checkpatch.pl` configuration
- Kconfig integration for kernel menuconfig

## Cursor & Copilot Rules

### Current Status
No Cursor rules (`.cursor/rules/` or `.cursorrules`) or Copilot instructions (`.github/copilot-instructions.md`) exist in this project.

### Recommended Rules
If adding AI assistant rules, consider:

```cursorrules
# GPIO Driver Rules
- This is a Linux kernel module project
- Follow Linux kernel coding style
- Use kernel-specific APIs and error handling
- Prioritize stability and compatibility
- Document device tree bindings
```

## Development Workflow

### 1. Code Changes
- Edit `gpio_customize.c` following kernel coding style
- Update `Makefile` if adding new source files
- Document changes in commit messages

### 2. Build Verification
```bash
# From kernel source root
make M=/path/to/gpio_customize modules
```

### 3. Load and Test
```bash
sudo insmod gpio_customize.ko
# Verify in /sys/class/gpio_customize/
# Test GPIO control
sudo rmmod gpio_customize
```

### 4. Debugging
- Check `dmesg` for kernel messages
- Use `dev_dbg()` for additional debug output (enable with `DYNAMIC_DEBUG`)

## Common Pitfalls

1. **Wrong kernel version**: Build against correct kernel headers
2. **Missing dependencies**: Ensure GPIO subsystem is enabled in kernel config
3. **Device tree errors**: Verify compatible string and GPIO specifications
4. **Memory leaks**: Use `devm_*` APIs for automatic resource management
5. **Race conditions**: Consider concurrent access to sysfs files

## License and Legal

- **License**: GPL-2.0-only (same as Linux kernel)
- **SPDX header**: Required in all source files (`// SPDX-License-Identifier: GPL-2.0-only`)
- **Copyright notices**: Include year and author

---

*This document was generated based on analysis of the existing codebase. Update it as the project evolves.*
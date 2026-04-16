# GPIO Customize Driver

## 概述

这是一个仿照Linux内核`leds-gpio.c`驱动实现的通用GPIO操作驱动。它提供了与LED子系统类似的sysfs接口，允许通过设备树配置GPIO引脚并在用户空间进行控制。

## 功能特性

- 仿照`gpio-leds`的设备树配置语法
- 每个GPIO在`/sys/class/gpio_customize/`下创建独立的设备目录
- 支持`value`文件读写（0或1）控制GPIO电平
- 支持`label`属性自定义设备名称

## 文件结构

```
gpio_customize/
├── gpio_customize.c      # 驱动核心代码
├── Makefile              # 构建配置
└── README.md             # 本文档
```

## 设备树配置

### 基本语法

```dts
gpio_customize {
    compatible = "gpio-customize";
    
    gpio-customize0 {
        label = "custom-gpio0";
        gpios = <&gpio 16 GPIO_ACTIVE_HIGH>;
    };
    
    gpio-customize1 {
        label = "custom-gpio1";
		gpios = <&gpio 17 GPIO_ACTIVE_HIGH>;
    };
};
```

### 属性说明

- `compatible`: 必须为`"gpio-customize"`
- 子节点命名: `gpio-customize#`（#为数字，如gpio-customize0, gpio-customize1等）
- `gpios`: GPIO描述符（同gpio-leds语法）
- `label`: 可选，设备显示名称（默认为节点名）
- `default-state`: 可选，初始状态（"off"、"on" 或 "keep"）

### default-state 属性详解

`default-state` 属性用于指定GPIO的初始状态，类似于LED子系统中的同名属性。

#### 可选值

- `"off"`（默认）: GPIO初始化为低电平
- `"on"`: GPIO初始化为高电平
- `"keep"`: 保持GPIO当前状态（读取当前值并设置为输出）

#### 设备树配置示例

```dts
gpio-customize0 {
    label = "gpio-default-off";
    gpios = <&pio 20 GPIO_ACTIVE_HIGH>;
    default-state = "off";
    /* 默认状态：低电平 */
};

gpio-customize1 {
    label = "gpio-default-on";
    gpios = <&pio 21 GPIO_ACTIVE_HIGH>;
    default-state = "on";
    /* 默认状态：高电平 */
};

gpio-customize2 {
    label = "gpio-keep-state";
    gpios = <&pio 22 GPIO_ACTIVE_HIGH>;
    default-state = "keep";
    /* 保持当前状态 */
};
```

## 用户空间使用

加载驱动后，每个配置的GPIO会在sysfs中创建相应接口：

```bash
# 查看所有gpio_customize设备
ls /sys/class/gpio_customize/

# 读取GPIO当前值
cat /sys/class/gpio_customize/custom-gpio0/value

# 设置GPIO为高电平
echo 1 > /sys/class/gpio_customize/custom-gpio0/value

# 设置GPIO为低电平  
echo 0 > /sys/class/gpio_customize/custom-gpio0/value
```

## 与gpio-leds对比

| 特性 | gpio-leds | gpio-customize |
|------|-----------|----------------|
| sysfs路径 | `/sys/class/leds/` | `/sys/class/gpio_customize/` |
| 控制文件 | `brightness` | `value` |
| 设备命名 | `led#` 或自定义label | `gpio-customize#` 或自定义label |
| 功能 | LED控制（亮度、触发器等） | GPIO电平控制（0/1） |
| 设备树兼容性 | `"gpio-leds"` | `"gpio-customize"` |

## 内核配置

### 菜单配置
```
Device Drivers  --->
    [*] GPIO Customize driver
```

### 手动配置
```bash
# 编译为模块
CONFIG_GPIO_CUSTOMIZE=m

# 编译进内核
CONFIG_GPIO_CUSTOMIZE=y
```

## 构建与安装

```bash
# 在内核源码根目录
make M=drivers/gpio_customize modules

# 安装模块
sudo insmod gpio_customize.ko
```

## 移植说明

如果您已有使用`gpio-leds`的代码，移植到`gpio-customize`只需：

1. **设备树**: 将`compatible = "gpio-leds"`改为`compatible = "gpio-customize"`
2. **节点命名**: 将`led#`节点改为`gpio-customize#`
3. **用户空间**: 将`/sys/class/leds/`路径改为`/sys/class/gpio_customize/`
4. **控制文件**: 将`brightness`文件改为`value`文件

## 示例

完整的设备树示例请参考`example.dts`文件。

## 注意事项

1. 驱动仅支持输出模式GPIO
2. 仅支持0/1两种状态（与LED的brightness 0/1对应）
3. 需要内核配置支持`GPIOLIB`和`OF`（设备树）
4. 驱动使用`devm_*`资源管理，简化错误处理

## 许可证

GPL-2.0-only（与Linux内核一致）
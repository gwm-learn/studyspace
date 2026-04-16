# GPIO Customize 驱动架构分析

## 概述

GPIO Customize 是一个 Linux 内核驱动模块，提供类似 LED 子系统的 sysfs 接口用于控制 GPIO 引脚。该驱动仿照 `gpio-leds` 的设计模式，但在 `/sys/class/gpio_customize/` 目录下创建设备，提供更通用的 GPIO 控制功能。

**核心功能**：
- 通过设备树配置多个 GPIO
- 每个 GPIO 在 sysfs 中创建独立的设备目录
- 通过 `value` 文件读写 GPIO 电平（0/1）
- 支持初始状态配置（off/on/keep）

## 架构组件

### 1. 驱动类型
- **平台驱动（Platform Driver）**：使用 `module_platform_driver()` 注册
- **设备树驱动（DT-based）**：通过 `of_match_table` 与设备树节点匹配
- **sysfs 类驱动**：创建自定义设备类 `/sys/class/gpio_customize/`

### 2. 核心组件关系
```
Linux Kernel
├── Platform Bus
│   └── gpio-customize driver (platform_driver)
├── Device Tree
│   └── gpio-customize nodes
├── GPIO Subsystem
│   └── GPIO descriptors
└── SysFS Class
    └── gpio_customize class
        └── gpio_customize devices
            └── value attribute
```

### 3. 组件详细说明

#### 3.1 平台驱动 (`gpio_customize_driver`)
```c
static struct platform_driver gpio_customize_driver = {
    .probe    = gpio_customize_probe,
    .remove   = gpio_customize_remove,
    .driver   = {
        .name           = "gpio-customize",
        .of_match_table = of_gpio_customize_match,
    },
};
```

#### 3.2 设备树匹配表 (`of_gpio_customize_match`)
```c
static const struct of_device_id of_gpio_customize_match[] = {
    { .compatible = "gpio-customize", },
    {},
};
MODULE_DEVICE_TABLE(of, of_gpio_customize_match);
```

#### 3.3 SysFS 类 (`gpio_customize_class`)
```c
static struct class gpio_customize_class = {
    .name        = "gpio_customize",
    .dev_groups  = gpio_customize_groups,
    .dev_release = gpio_customize_release,
};
```

#### 3.4 属性组 (`gpio_customize_groups`)
```c
static const struct attribute_group *gpio_customize_groups[] = {
    &gpio_customize_group,
    NULL,
};
```

#### 3.5 设备属性 (`gpio_customize_attrs`)
```c
static struct attribute *gpio_customize_attrs[] = {
    &dev_attr_value.attr,  // 唯一的属性：value 文件
    NULL,
};
```

## 数据结构

### 1. 主要数据结构

#### `struct gpio_customize_data` - 单个 GPIO 设备数据
```c
struct gpio_customize_data {
    struct device *dev;                    // 关联的设备对象
    struct gpio_desc *gpiod;               // GPIO 描述符
    char name[GPIO_CUSTOMIZE_MAX_NAME_LEN]; // 设备名称
    u8 can_sleep;                          // GPIO 操作是否可休眠
    enum gpio_customize_default_state default_state; // 默认状态
};
```

**字段说明**：
- `dev`: 在 `gpio_customize` 类下创建的设备对象
- `gpiod`: Linux GPIO 子系统提供的 GPIO 描述符
- `name`: 设备名称，来自设备树 `label` 属性或节点名
- `can_sleep`: 标记 GPIO 操作是否需要休眠上下文
- `default_state`: GPIO 初始状态（OFF/ON/KEEP）

#### `struct gpio_customize_priv` - 驱动私有数据
```c
struct gpio_customize_priv {
    int num_gpios;                     // GPIO 设备数量
    struct gpio_customize_data gpios[]; // 柔性数组存储所有 GPIO 数据
};
```

**设计模式**：使用柔性数组管理可变数量的 GPIO 设备，内存连续分配提高缓存效率。

### 2. 枚举类型

#### `enum gpio_customize_default_state`
```c
enum gpio_customize_default_state {
    GPIO_CUSTOMIZE_DEFSTATE_OFF = 0,   // 默认低电平
    GPIO_CUSTOMIZE_DEFSTATE_ON  = 1,   // 默认高电平
    GPIO_CUSTOMIZE_DEFSTATE_KEEP = 2,  // 保持当前状态
};
```

### 3. 常量定义
```c
#define GPIO_CUSTOMIZE_MAX_NAME_LEN 32  // 设备名称最大长度
```

## 工作流程

### 1. 驱动初始化流程

**调用链**：
```
module_platform_driver(gpio_customize_driver)
    ├── platform_driver_register()
    │   ├── driver_register()
    │   └── 驱动添加到平台总线
    └── 等待匹配的设备树节点
```

**关键点**：
- 使用 `module_platform_driver()` 宏简化驱动注册
- 驱动注册为平台驱动，通过 compatible 字符串匹配
- 延迟初始化：直到匹配的设备出现才执行 probe

### 2. 设备探测流程 (gpio_customize_probe)

**步骤**：
1. **类注册**：`class_register(&gpio_customize_class)`
   - 创建 `/sys/class/gpio_customize/` 目录
   - 失败时直接返回错误

2. **GPIO 设备创建**：`gpio_customize_create(pdev)`
   - 统计设备树子节点数量：`device_get_child_node_count()`
   - 分配私有数据结构：`devm_kzalloc()` 带柔性数组
   - 遍历每个子节点创建 GPIO 设备

3. **数据关联**：`platform_set_drvdata(pdev, priv)`
   - 将私有数据关联到平台设备
   - 便于后续操作和清理

4. **日志输出**：`dev_info()` 报告注册的设备数量

### 3. GPIO 设备创建流程 (create_gpio_customize)

**详细步骤**：
1. **名称获取**：
   ```c
   fwnode_property_read_string(fwnode, "label", &name);
   if (!name) name = fwnode_get_name(fwnode);
   if (!name) name = "gpio_customize";
   strscpy(data->name, name, GPIO_CUSTOMIZE_MAX_NAME_LEN);
   ```

2. **GPIO 获取**：
   ```c
   data->gpiod = devm_fwnode_get_gpiod_from_child(parent, NULL, fwnode,
                                                GPIOD_ASIS, NULL);
   if (IS_ERR(data->gpiod)) {
       dev_err(parent, "Failed to get GPIO for %s: %ld\n",
               name, PTR_ERR(data->gpiod));
       return PTR_ERR(data->gpiod);
   }
   ```

3. **休眠能力检测**：`data->can_sleep = gpiod_cansleep(data->gpiod);`

4. **默认状态解析**：
   ```c
   data->default_state = gpio_customize_init_default_state_get(fwnode);
   ```
   - 读取 `default-state` 属性
   - 支持值："off"（默认）、"on"、"keep"

5. **GPIO 方向配置**：
   - **KEEP 状态**：先设置为输入，读取当前值，再设置为输出
   - **ON/OFF 状态**：直接设置为输出，初始值相应为 1/0

6. **设备创建**：
   ```c
   data->dev = device_create(&gpio_customize_class, parent,
                             0, NULL, "%s", data->name);
   ```

7. **数据关联**：`dev_set_drvdata(data->dev, data);`

### 4. SysFS 操作流程

#### 4.1 GPIO 值读取 (value_show)
```c
static ssize_t value_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct gpio_customize_data *data = dev_get_drvdata(dev);
    int value;
    
    if (data->can_sleep)
        value = gpiod_get_value_cansleep(data->gpiod);
    else
        value = gpiod_get_value(data->gpiod);
    
    if (value < 0)
        return value;
    
    return sprintf(buf, "%d\n", value);
}
```

**数据流**：
```
用户空间 cat /sys/class/gpio_customize/<name>/value
    ↓
内核空间 value_show() 被调用
    ↓
通过设备指针获取 gpio_customize_data
    ↓
根据 can_sleep 选择适当的 GPIO 读取函数
    ↓
检查返回值（负值为错误）
    ↓
格式化输出到用户缓冲区
```

#### 4.2 GPIO 值设置 (value_store)
```c
static ssize_t value_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t size)
{
    struct gpio_customize_data *data = dev_get_drvdata(dev);
    unsigned long value;
    int ret;
    
    ret = kstrtoul(buf, 0, &value);
    if (ret)
        return ret;
    
    /* 只允许 0 或 1 */
    if (value > 1)
        return -EINVAL;
    
    if (data->can_sleep)
        gpiod_set_value_cansleep(data->gpiod, value);
    else
        gpiod_set_value(data->gpiod, value);
    
    return size;
}
```

**数据流**：
```
用户空间 echo 1 > /sys/class/gpio_customize/<name>/value
    ↓
内核空间 value_store() 被调用
    ↓
解析用户输入为无符号长整型
    ↓
验证值范围（0 或 1）
    ↓
根据 can_sleep 选择适当的 GPIO 设置函数
    ↓
返回操作结果
```

### 5. 设备移除流程 (gpio_customize_remove)

**步骤**：
1. **获取私有数据**：`priv = platform_get_drvdata(pdev);`
2. **遍历所有 GPIO 设备**：
   ```c
   for (i = 0; i < priv->num_gpios; i++) {
       struct gpio_customize_data *data = &priv->gpios[i];
       if (data->dev)
           device_unregister(data->dev);
   }
   ```
3. **注销类**：`class_unregister(&gpio_customize_class);`

**关键点**：
- 反向执行 probe 中的操作
- 使用 `device_unregister()` 删除 sysfs 设备
- 类在最后一个设备移除后注销

## 内核集成

### 1. 依赖的内核子系统

| 子系统 | 头文件 | 主要用途 |
|--------|--------|----------|
| GPIO 子系统 | `<linux/gpio/consumer.h>` | GPIO 描述符获取和操作 |
| 设备树 (OF) | `<linux/of.h>` | 设备树节点解析 |
| 平台设备 | `<linux/platform_device.h>` | 平台驱动框架 |
| 设备模型 | `<linux/device.h>` | 设备类和管理 |
| 属性管理 | `<linux/property.h>` | 设备树属性读取 |
| 内存管理 | `<linux/slab.h>` | 动态内存分配 |
| 错误处理 | `<linux/err.h>` | 错误指针处理 |

### 2. 主要内核 API 使用

#### 资源管理 API
- `devm_kzalloc()`: 设备托管的内存分配
- `devm_fwnode_get_gpiod_from_child()`: 设备托管的 GPIO 获取
- 自动释放：当设备被移除时，资源自动释放

#### GPIO API
- `gpiod_get_value()` / `gpiod_get_value_cansleep()`: 读取 GPIO 值
- `gpiod_set_value()` / `gpiod_set_value_cansleep()`: 设置 GPIO 值
- `gpiod_direction_output()`: 设置为输出模式
- `gpiod_direction_input()`: 设置为输入模式
- `gpiod_cansleep()`: 检查 GPIO 操作是否需要休眠上下文

#### 设备树 API
- `fwnode_property_read_string()`: 读取字符串属性
- `fwnode_get_name()`: 获取节点名称
- `device_get_child_node_count()`: 统计子节点数量
- `device_for_each_child_node()`: 遍历子节点

#### SysFS API
- `class_register()` / `class_unregister()`: 注册/注销设备类
- `device_create()` / `device_unregister()`: 创建/删除设备
- `DEVICE_ATTR_RW()`: 定义可读写的设备属性

## 设备树绑定

### 1. 兼容性字符串
```dts
compatible = "gpio-customize";
```

### 2. 节点结构
```dts
gpio_customize {
    compatible = "gpio-customize";
    
    gpio-customize0 {
        label = "custom-gpio0";           // 可选，设备名称
        gpios = <&gpio 16 GPIO_ACTIVE_HIGH>; // 必需，GPIO 定义
        default-state = "off";            // 可选，默认状态
    };
    
    gpio-customize1 {
        label = "custom-gpio1";
        gpios = <&gpio 17 GPIO_ACTIVE_HIGH>;
        default-state = "on";
    };
    
    gpio-customize2 {
        label = "keep-state-gpio";
        gpios = <&gpio 18 GPIO_ACTIVE_HIGH>;
        default-state = "keep";           // 保持当前状态
    };
};
```

### 3. 属性说明

| 属性 | 必需 | 类型 | 说明 |
|------|------|------|------|
| `gpios` | 是 | GPIO 描述符 | 指定 GPIO 引脚，格式同 `gpio-leds` |
| `label` | 否 | 字符串 | 设备显示名称，默认为节点名 |
| `default-state` | 否 | 字符串 | 初始状态："off"、"on"、"keep" |

### 4. `default-state` 语义

| 值 | 行为 | 应用场景 |
|----|------|----------|
| "off" | GPIO 初始化为低电平输出 | 默认安全状态 |
| "on" | GPIO 初始化为高电平输出 | 设备使能状态 |
| "keep" | 读取当前值并设置为输出 | 状态保持，热重启 |

**"keep" 状态的实现**：
1. 将 GPIO 设置为输入模式
2. 读取当前电平值
3. 将 GPIO 设置为输出模式，使用读取的值作为初始值

## 错误处理机制

### 1. 错误返回模式
- **整数返回**：负的错误码（如 `-EINVAL`, `-ENOMEM`, `-ENODEV`）
- **指针返回**：使用 `ERR_PTR()` 包装错误码，用 `IS_ERR()` 检查

### 2. 错误处理层次

#### 2.1 GPIO 获取失败
```c
if (IS_ERR(data->gpiod)) {
    dev_err(parent, "Failed to get GPIO for %s: %ld\n",
            name, PTR_ERR(data->gpiod));
    return PTR_ERR(data->gpiod);
}
```

#### 2.2 设备创建失败
```c
if (IS_ERR(data->dev)) {
    ret = PTR_ERR(data->dev);
    dev_err(parent, "Failed to create device %s: %d\n", data->name, ret);
    data->dev = NULL;
    return ret;
}
```

#### 2.3 资源分配失败
```c
if (!priv)
    return ERR_PTR(-ENOMEM);
```

### 3. 资源清理策略

#### 3.1 成功路径
- 使用 `devm_*` API 分配的资源在设备移除时自动释放
- 手动创建的资源（设备、类）在 remove 函数中清理

#### 3.2 错误路径（部分失败）
```c
ret = create_gpio_customize(child, gpio_data, dev);
if (ret < 0) {
    /* 清理已创建的设备 */
    while (priv->num_gpios > 0) {
        struct gpio_customize_data *data = &priv->gpios[priv->num_gpios - 1];
        if (data->dev)
            device_unregister(data->dev);
        priv->num_gpios--;
    }
    fwnode_handle_put(child);
    return ERR_PTR(ret);
}
```

**设计原则**：错误发生时，清理已分配的资源，保持系统状态一致。

## 并发和电源管理考虑

### 1. 并发访问处理
- **无显式锁**：当前实现假设 GPIO 操作为原子操作
- **潜在问题**：多个进程同时读写同一 GPIO 可能产生竞态条件
- **建议改进**：添加互斥锁保护 GPIO 操作序列

### 2. 休眠上下文处理
- **`can_sleep` 检测**：`gpiod_cansleep()` 确定 GPIO 操作是否需要休眠
- **条件执行**：根据 `can_sleep` 选择适当的 API 变体
- **意义**：确保在原子上下文中不使用可能休眠的操作

### 3. 电源管理
- **当前状态**：未实现电源管理回调（suspend/resume）
- **潜在需求**：系统挂起时保存/恢复 GPIO 状态
- **扩展建议**：添加 `pm_ops` 支持

## 扩展建议

### 1. 功能扩展
1. **方向控制**：添加 `direction` 文件，支持输入/输出模式切换
2. **中断支持**：添加 GPIO 中断处理，支持边缘检测
3. **脉宽调制**：添加 PWM 功能支持
4. **多值支持**：支持多级输出（非二进制 GPIO）

### 2. 健壮性改进
1. **并发保护**：添加互斥锁保护关键操作
2. **输入验证**：增强用户输入验证和边界检查
3. **状态跟踪**：添加设备状态跟踪和验证

### 3. 测试增强
1. **单元测试**：添加 KUnit 测试用例
2. **集成测试**：创建设备树测试覆盖
3. **压力测试**：并发访问和错误注入测试

### 4. 文档完善
1. **内核文档**：添加 `Documentation/devicetree/bindings/gpio/gpio-customize.yaml`
2. **API 文档**：使用 kernel-doc 格式注释函数
3. **示例代码**：提供更多使用示例

## 总结

GPIO Customize 驱动是一个设计良好的 Linux 内核模块，具有以下特点：

1. **架构清晰**：严格遵循 Linux 平台驱动框架
2. **资源管理**：充分利用 `devm_*` API 简化资源管理
3. **设备树集成**：完整的设备树绑定支持
4. **用户接口**：简单直观的 sysfs 接口
5. **错误处理**：全面的错误检查和资源清理

该驱动成功地将 GPIO 控制抽象为类似 LED 子系统的接口，为通用 GPIO 控制提供了标准化解决方案。其模块化设计和清晰的层次结构使得扩展和维护相对容易。

---

**分析完成时间**：2026-04-16  
**分析工具**：OpenCode AI Agent  
**代码版本**：基于 gpio_customize.c 文件分析  
**内核版本要求**：支持设备树和 GPIO 描述符的现代 Linux 内核
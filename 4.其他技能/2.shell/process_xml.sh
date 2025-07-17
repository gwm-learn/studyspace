#!/bin/bash

# 检查参数数量
if [ $# -ne 2 ]; then
    echo "错误: 需要提供两个参数" >&2
    echo "用法: $0 <输入XML文件> <输出XML文件>" >&2
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

# 检查输入文件
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件 '$INPUT_FILE' 不存在" >&2
    exit 1
fi

if [ ! -r "$INPUT_FILE" ]; then
    echo "错误: 输入文件 '$INPUT_FILE' 不可读" >&2
    exit 1
fi

# 检查输出目录
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
if [ ! -d "$OUTPUT_DIR" ]; then
    echo "错误: 输出目录 '$OUTPUT_DIR' 不存在" >&2
    exit 1
fi

if [ ! -w "$OUTPUT_DIR" ]; then
    echo "错误: 输出目录 '$OUTPUT_DIR' 不可写" >&2
    exit 1
fi

# 处理XML并生成结果
awk '
BEGIN {
    stack_top = 0  # 栈顶指针
    # 类型映射关系
    type_map["s"] = "string"
    type_map["s16"] = "string"
    type_map["s32"] = "string"
    type_map["s64"] = "string"
    type_map["s128"] = "string"
    type_map["s256"] = "string"
    type_map["bool"] = "boolean"
    type_map["uint"] = "unsignedInt"
    # 默认值映射
    default_val["string"] = ""
    default_val["boolean"] = "0"
    default_val["unsignedInt"] = "0"
    # 输出XML头部
    print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    print "<ParameterList xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
}

# 处理对象开始标签（非自闭合标签）
/<object[^>]*name="[^"]*"[^>]*>/ && !/\/>/ {
    if (match($0, /name="([^"]*)"/, arr)) {
        name = arr[1]
        stack[stack_top] = name
        stack_top++
    }
    next
}

# 处理对象结束标签
/<\/object>/ {
    if (stack_top > 0) stack_top--
    next
}

# 处理参数标签（匹配所有带name属性的param，不强制要求type）
/<param[^>]*name="[^"]+"[^>]*>/ {
    if (stack_top == 0) next  # 不在有效对象层级内则跳过

    # 提取参数名（必须存在）
    if (!match($0, /name="([^"]+)"/, arr)) {
        next  # 无name属性则跳过
    }
    param_name = arr[1]
    if (param_name == "") next  # 空name跳过

    # 提取参数类型（不存在则用默认值）
    param_type = ""
    if (match($0, /type="([^"]+)"/, arr)) {
        param_type = arr[1]
    }

    # 映射到标准类型（不存在的类型默认string）
    if (param_type in type_map) {
        std_type = type_map[param_type]
    } else {
        std_type = "string"  # 无type或未知type都默认string
    }

    # 构建完整路径
    path = ""
    for (i = 0; i < stack_top; i++) {
        if (i > 0) path = path "."
        path = path stack[i]
    }
    full_path = path "." param_name

    # 输出结构化数据
    print "    <ParameterValueStruct>"
    print "        <Name>" full_path "</Name>"
    print "        <Value xsi:type=\"xsd:" std_type "\">" default_val[std_type] "</Value>"
    print "    </ParameterValueStruct>"
}

END {
    print "</ParameterList>"
}
' "$INPUT_FILE" > "$OUTPUT_FILE"

# 检查处理结果
if [ $? -eq 0 ]; then
    echo "处理完成，结果已保存到 '$OUTPUT_FILE'"
else
    echo "错误: 处理过程中发生错误" >&2
    exit 1
fi
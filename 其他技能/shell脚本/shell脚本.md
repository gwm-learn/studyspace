# 运算符
## 算数运算符

```
+ 	加法 	`expr $a + $b` 结果为 30。
- 	减法 	`expr $a - $b` 结果为 10。
* 	乘法 	`expr $a \* $b` 结果为  200。
/ 	除法 	`expr $b / $a` 结果为 2。
% 	取余 	`expr $b % $a` 结果为 0。
= 	赋值 	a=$b 将把变量 b 的值赋给 a。
== 	相等。用于比较两个数字，相同则返回 true。 	[ $a == $b ] 返回 false。
!= 	不相等。用于比较两个数字，不相同则返回 true。 	[ $a != $b ] 返回 true


#!/bin/sh
a=10
b=20
val=`expr $a + $b`
echo "a + b : $val"
val=`expr $a - $b`
echo "a - b : $val"
val=`expr $a \* $b`
echo "a * b : $val"
val=`expr $b / $a`
echo "b / a : $val"
val=`expr $b % $a`
echo "b % a : $val"
if [ $a == $b ]
then
    echo "a is equal to b"
fi
if [ $a != $b ]
then
    echo "a is not equal to b"
fi

```

## 关系运算符
```
-eq 	检测两个数是否相等，相等返回 true。 	[ $a -eq $b ] 返回 true。
-ne 	检测两个数是否相等，不相等返回 true。 	[ $a -ne $b ] 返回 true。
-gt 	检测左边的数是否大于右边的，如果是，则返回 true。 	[ $a -gt $b ] 返回 false。
-lt 	检测左边的数是否小于右边的，如果是，则返回 true。 	[ $a -lt $b ] 返回 true。
-ge 	检测左边的数是否大等于右边的，如果是，则返回 true。 	[ $a -ge $b ] 返回 false。
-le 	检测左边的数是否小于等于右边的，如果是，则返回 true。 	[ $a -le $b ] 返回 true。

#!/bin/sh
a=10
b=20
if [ $a -eq $b ]
then
    echo "$a -eq $b : a is equal to b"
else
    echo "$a -eq $b: a is not equal to b"
fi
if [ $a -ne $b ]
then
    echo "$a -ne $b: a is not equal to b"
else
    echo "$a -ne $b : a is equal to b"
fi
if [ $a -gt $b ]
then
    echo "$a -gt $b: a is greater than b"
else
    echo "$a -gt $b: a is not greater than b"
fi
if [ $a -lt $b ]
then
    echo "$a -lt $b: a is less than b"
else
    echo "$a -lt $b: a is not less than b"
fi
if [ $a -ge $b ]
then
    echo "$a -ge $b: a is greater or  equal to b"
else
    echo "$a -ge $b: a is not greater or equal to b"
fi
if [ $a -le $b ]
then
    echo "$a -le $b: a is less or  equal to b"
else
    echo "$a -le $b: a is not less or equal to b"
fi
```

## 布尔运算符
```
! 	非运算，表达式为 true 则返回 false，否则返回 true。 	[ ! false ] 返回 true。
-o 	或运算，有一个表达式为 true 则返回 true。 	[ $a -lt 20 -o $b -gt 100 ] 返回 true。
-a 	与运算，两个表达式都为 true 才返回 true。 	[ $a -lt 20 -a $b -gt 100 ] 返回 false。

#!/bin/sh
a=10
b=20
if [ $a != $b ]
then
    echo "$a != $b : a is not equal to b"
else
    echo "$a != $b: a is equal to b"
fi
if [ $a -lt 100 -a $b -gt 15 ]
then
    echo "$a -lt 100 -a $b -gt 15 : returns true"
else
    echo "$a -lt 100 -a $b -gt 15 : returns false"
fi
if [ $a -lt 100 -o $b -gt 100 ]
then
    echo "$a -lt 100 -o $b -gt 100 : returns true"
else
    echo "$a -lt 100 -o $b -gt 100 : returns false"
fi
if [ $a -lt 5 -o $b -gt 100 ]
then
    echo "$a -lt 100 -o $b -gt 100 : returns true"
else
    echo "$a -lt 100 -o $b -gt 100 : returns false"
fi
```

## 字符串运算符
```
= 	检测两个字符串是否相等，相等返回 true。 	[ $a = $b ] 返回 false。
!= 	检测两个字符串是否相等，不相等返回 true。 	[ $a != $b ] 返回 true。
-z 	检测字符串长度是否为0，为0返回 true。 	[ -z $a ] 返回 false。
-n 	检测字符串长度是否为0，不为0返回 true。 	[ -z $a ] 返回 true。
str 	检测字符串是否为空，不为空返回 true。 	[ $a ] 返回 true。

#!/bin/sh
a="abc"
b="efg"
if [ $a = $b ]
then
    echo "$a = $b : a is equal to b"
else
    echo "$a = $b: a is not equal to b"
fi
if [ $a != $b ]
then
    echo "$a != $b : a is not equal to b"
else
    echo "$a != $b: a is equal to b"
fi
if [ -z $a ]
then
    echo "-z $a : string length is zero"
else
    echo "-z $a : string length is not zero"
fi
if [ -n $a ]
then
    echo "-n $a : string length is not zero"
else
    echo "-n $a : string length is zero"
fi
if [ $a ]
then
    echo "$a : string is not empty"
else
    echo "$a : string is empty"
fi
```

## 文件测试运算符
```
-b file 	检测文件是否是块设备文件，如果是，则返回 true。 	[ -b $file ] 返回 false。
-c file 	检测文件是否是字符设备文件，如果是，则返回 true。 	[ -b $file ] 返回 false。
-d file 	检测文件是否是目录，如果是，则返回 true。 	[ -d $file ] 返回 false。
-f file 	检测文件是否是普通文件（既不是目录，也不是设备文件），如果是，则返回 true。 	[ -f $file ] 返回 true。
-g file 	检测文件是否设置了 SGID 位，如果是，则返回 true。 	[ -g $file ] 返回 false。
-k file 	检测文件是否设置了粘着位(Sticky Bit)，如果是，则返回 true。 	[ -k $file ] 返回 false。
-p file 	检测文件是否是具名管道，如果是，则返回 true。 	[ -p $file ] 返回 false。
-u file 	检测文件是否设置了 SUID 位，如果是，则返回 true。 	[ -u $file ] 返回 false。
-r file 	检测文件是否可读，如果是，则返回 true。 	[ -r $file ] 返回 true。
-w file 	检测文件是否可写，如果是，则返回 true。 	[ -w $file ] 返回 true。
-x file 	检测文件是否可执行，如果是，则返回 true。 	[ -x $file ] 返回 true。
-s file 	检测文件是否为空（文件大小是否大于0），不为空返回 true。 	[ -s $file ] 返回 true。
-e file 	检测文件（包括目录）是否存在，如果是，则返回 true。 	[ -e $file ] 返回 true。

#!/bin/sh
file="/var/www/tutorialspoint/unix/test.sh"
if [ -r $file ]
then
    echo "File has read access"
else
    echo "File does not have read access"
fi
if [ -w $file ]
then
    echo "File has write permission"
else
    echo "File does not have write permission"
fi
if [ -x $file ]
then
    echo "File has execute permission"
else
    echo "File does not have execute permission"
fi
if [ -f $file ]
then
    echo "File is an ordinary file"
else
    echo "This is sepcial file"
fi
if [ -d $file ]
then
    echo "File is a directory"
else
    echo "This is not a directory"
fi
if [ -s $file ]
then
    echo "File size is zero"
else
    echo "File size is not zero"
fi
if [ -e $file ]
then
    echo "File exists"
else
    echo "File does not exist"
fi
```

#  逻辑结构
## if语句
* if语句
```
if [ expression ]
then
   Statement(s) to be executed if expression is true
fi
```
* if else语句
```
if [ expression ]
then
   Statement(s) to be executed if expression is true
else
   Statement(s) to be executed if expression is not true
fi
```
* if elif语句
```
if [ expression 1 ]
then
   Statement(s) to be executed if expression 1 is true
elif [ expression 2 ]
then
   Statement(s) to be executed if expression 2 is true
elif [ expression 3 ]
then
   Statement(s) to be executed if expression 3 is true
else
   Statement(s) to be executed if no expression is true
fi
```

## case语句
```
case 值 in
模式1)
    command1
    command2
    command3
    ;;
模式2）
    command1
    command2
    command3
    ;;
*)
    command1
    command2
    command3
    ;;
esac
```

## for语句
```
for 变量 in 列表
do
    command1
    command2
    ...
    commandN
done
```

## while语句
```
while command
do
   Statement(s) to be executed if command is true
done
```

## until
```
until command
do
   Statement(s) to be executed until command is true
done
```

## 跳出循环
```
break
在嵌套循环中，break 命令后面还可以跟一个整数，表示跳出第几层循环。例如：
break n
```

# 函数
```
定义：
function_name () {
    list of commands
    [ return value ]
}
或者：
function function_name () {
    list of commands
    [ return value ]
}

调用：
#!/bin/bash
funWithReturn(){
    echo "The function is to get the sum of two numbers..."
    echo -n "Input first number: "
    read aNum
    echo -n "Input another number: "
    read anotherNum
    echo "The two numbers are $aNum and $anotherNum !"
    return $(($aNum+$anotherNum))
}
funWithReturn
# Capture value returnd by last command
ret=$?
echo "The sum of two numbers is $ret !"

删除函数：
$unset .f function_name

函数参数：
#!/bin/bash
funWithParam(){
    echo "The value of the first parameter is $1 !"
    echo "The value of the second parameter is $2 !"
    echo "The value of the tenth parameter is $10 !"
    echo "The value of the tenth parameter is ${10} !"
    echo "The value of the eleventh parameter is ${11} !"
    echo "The amount of the parameters is $# !"  # 参数个数
    echo "The string of the parameters is $* !"  # 传递给函数的所有参数
}
funWithParam 1 2 3 4 5 6 7 8 9 34 73
```


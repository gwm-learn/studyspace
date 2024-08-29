# 介绍
Svn的全名是Subversion，它使用服务端—客户端的结构，当然服务端与客户端可以都运行在同一台服务器上。在服务端是存放着所有受控制数据的Subversion仓库，另一端是Subversion的客户端程序，管理着受控数据的一部分在本地的映射(称为工作副本)。在这两端之间，是通过各种仓库存取层(RepositoryAccess，简称RA)的多条通道进行访问的。这些通道中，可以通过不同的网络协议，例如HTTP、SSH等，或本地文件的方式来对仓库进行操作。   

Subversion是一种开放源码的全新版本控制系统，支持可在本地访问或通过网络访问的数据库和文件系统存储库。不但提供了常见的比较、修补、标记、提交、回复和分支功能性，Subversion还增加了追踪移动和删除的能力。此外，它支持非ASCII文本和二进制数据，所有这一切都使Subversion不仅对传统的编程任务非常有用，同时也适于Web 开发、图书创作和其他在传统方式下未采纳版本控制功能的领域.   

1. 通过svn协议访问   
客户端若要通过svn协议访问仓库，必须在存放仓库的机器上运行svnserve服务程序。启动该程序后，会监听在3690端口，以响应客户端的访问工作。   

 

2. 通过HTTP协议访问版本库      
通过HTTP协议访问版本库是Subversion的亮点之一，这种方式具备许多svnserve服务器所没有的特性，使用上更加灵活。

# 使用
1. 将文件checkout到本地
```SHELL
svn co 远程仓库路径 本地目录 --username=xxx --password=xxx
```

2. 添加新文件
```SHELL
svn add 新文件名
```

3. 提交文件
```SHELL
svn ci path -m "提交信息" #path为要提交的文件路径
```

4. 查看本地变更
```SHELL
svn status #'?'问号表示未加到版本控制
```

5. 查看log
```SHELL
svn log
```
查看提交记录(限制数量)
```SHELL
svn log -l 10
```
查看某次提交的文件修改记录(只看修改了哪些文件，而不看具体的修改内容)
```SHELL
svn log -v -c r18370
```
不显示commit消息(可以与-v一起使用)
```SHELL
svn log -q -c r18370
```
xml显示提交
```SHELL
svn log -v --xml -c r18370
```
其他选项 -r(与-c参数功能一样，但比-c参数功能多)
```SHELL
svn log -v -r r18370
```
一段时间内的提交
```SHELL
svn log -r {2023-03-01}:{2023-03-21}
```
根据作者查看记录
```SHELL
svn log --search gaoweiming -v
```

6. 更新文件
```SHELL
svn up
```

7. 比较差异
```SHELL
svn diff filename
```
查看某次历史的提交的所有修改记录的话使用如下命令
```SHELL
svn diff -c r12347
```

8. 版本
单个文件版本退回
```SHELL
svn merge -r r18434:r18432 broadlink/package/dongle-mngr/files/led.sh # led.sh 从r18434退回到r18432
```
# 分支
```SHELL
git branch -a
```
## 查看分支之间关系图
```SHELL
git log --graph --decorate --oneline --simplify-by-decoration --all
```

# 查看远程仓库
```SHELL
git remote -v
git remote show origin
```

# 查看完整提交记录树
```SHELL
git log --oneline --graph --decorate --all
```

# 修改commit
```SHELL
# 修改最近提交的 commit 信息
$ git commit --amend --message="modify message by daodaotest" --author="jiangliheng <jiang_liheng@163.com>"

# 仅修改 message 信息
$ git commit --amend --message="modify message by daodaotest"

# 仅修改 author 信息
$ git commit --amend --author="jiangliheng <jiang_liheng@163.com>"
```

# 提交
## 本地分支名和远程分支名不一致
```SHELL
git push <远程主机名> <本地分支名>:<远程分支名>   
```

## 本地分支名和远程分支名一致
```SHELL
git push <远程主机名> <本地分支名>   
```

# 不同网站的同一个仓库合并在一个本地仓库
## 添加另一个远程仓库
```SHELL
git remote add bitbucket https://bitbucket.askey.com.tw:8443/scm/pgrtlvwdro/rtl6100vw.git   
git remote -v   
git pull bitbucket   
git branch -a   
```

# 创建本地分支
```SHELL
# 取远程分支并分化一个新分支：
git checkout -b mybranch origin/mybranch   
```
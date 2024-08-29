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
## Morris遍历

### 介绍

Morris遍历实现的是一种不使用栈结构来实现顺序上的可追溯性，而是通过树的叶节点为空，来实现空间开销上的极致削减。

### 伪代码实现

```c++
current = root
while current is not Null
    if not exists current.left
        visit(current)
        current = current.right
	else
        predecessor = findPredecessor(current)
        if not exists predecessor.right
            predecessor.right = current
            current = current.left
        else predecessor.rihgt = Null
            visit(current)
            current = current.right
```


## prim算法

- 与Dijastra、Bellman-Ford算法不同的是，它的heap+map存储的是每个点的相对短边
- 每次查完邻居，就可以做一个extratMin，记得更新记录
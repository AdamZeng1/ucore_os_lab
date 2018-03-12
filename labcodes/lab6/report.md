# Lab 6

## 知识点

这些是与本实验有关的原理课的知识点：

* 时间片轮转算法：不过原理课没有考虑实现上的细节，实验需要考虑

此外，本实验还涉及如下知识点：

* 调度器框架
* Stride Scheduling调度算法

遗憾的是，如下知识点在原理课中很重要，但本次实验没有很好的对应：

* 其他调度算法
* 实时调度
* 多处理器调度
* 优先级反置解决

## 练习0

首先，需要对之前的代码进行微小的修改：

1. 删除`ticks % TICK_NUM == 0`时，将当前进程的`need_resched`置1，因为这是临时的调度
2. 每一次时钟中断来临时调用`sched_class_proc_tick`函数，为此还需要将这个函数变为非`static`并在头文件中加入声明

与参考答案相比，参考答案忘记在时钟中断调用`sched_class_proc_tick`了。

## 练习1

总结调度器接口每个函数的作用或用法：

**init**



**enqueue**



**dequeue**



**pick_next**



**proc_tick**

每一次时钟中断，此函数会被调用，如果是基于时间片的调度算法，可以在此函数中减少当前进程的时间片。



## 练习2

Stride Scheduling调度算法的实现实际上是接口的实现，把接口实现正确，调度算法就可以正常工作了。

算法实现参考MOOC和注释的讲解，与练习1类似，有如下接口的实现需要说明：

**init**



**enqueue**



**dequeue**



**pick_next**



**proc_tick**



**BIG_STRIDE的选取**

记`max_stride`为某时刻所有`stride`真实值的最大值，`min_stride`为某时刻所有`stride`真实值的最小值，`PASS_MAX`为每一次`stride`增量的最大值。

对于每一个进程而言，考虑到优先级是`priority`正整数，每一次`stride`的增量`pass`有这样的关系：

`pass = BIG_STRIDE / priority <= BIG_STRIDE`

所以，`PASS_MAX <= BIG_STRIDE`

首先，利用数学归纳法证明`max_stride - min_stride <= PASS_MAX`：

不妨假设进程数多于一个，否则结论立即成立。

*基础*

起初，大家`stride`都为0，所以`max_stride = min_stride = 0`。

某个进程的`stride`被增加后，有`max_stride = pass <= PASS_MAX`，且仍有`min_stride = 0`。

于是`max_stride - min_stride = max_stride <= PASS_MAX`。

*归纳*

根据归纳假设，有`max_stride - min_stride <= PASS_MAX`，为了便于区分，加一个角标，改写为：

`max_stride0 - min_stride0 <= PASS_MAX`，0表示增加`stride`之前的状态

根据算法，原来`min_stride0`的进程需要被变为`min_stride0 + pass <= min_stride0 + PASS_MAX`

1 若`min_stride0 + pass <= max_stride0`，则`max_stride`不发生变化，而`min_stride`变大，所以

```
   max_stride - min_stride
 = max_stride0 - min_stride
<= max_stride0 - min_stride0
<= PASS_MAX
```

2 否则，若`min_stride0 + pass > max_stride0`，则`max_stride = min_stride0 + pass`，而`min_stride`变大，所以

```
   max_stride - min_stride
 = min_stride0 + pass - min_stride
<= min_stride0 + pass - min_stride0
 = pass
<= PASS_MAX
```

Q.E.D.

于是，`max_stride - min_stride <= PASS_MAX <= BIG_STRIDE`

接着，考虑`BIG_STRIDE`的选取。

由于算法将使用两进程`stride`的差值（结果转换为32位有符号数）进行大小判断，因此需要保证任意两个进程`stride`的差值在32位有符号数能够表示的范围之内，即`max_stride - min_stride <= 0x7fffffff`。

立即得到`BIG_STRIDE <= 0x7fffffff`。

我的实现与参考答案十分一致，不过没有实现链表的版本。
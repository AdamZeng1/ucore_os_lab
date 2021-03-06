# Lab 3

## 知识点

这些是与本实验有关的原理课的知识点：

* 先进先出替换算法：原理课注重原理，本实验注重实现
* 扩展时钟替换算法：原理课注重原理，本实验注重实现

此外，本实验还涉及如下知识点：

无

遗憾的是，如下知识点在原理课中很重要，但本次实验没有很好的对应：

* 覆盖技术
* 其他局部页替换算法
* 全局页替换算法
* Belady现象
* 抖动和负载控制

## 练习1

这个练习提供的注释已经写得非常详细，比实际要写的代码还要多，根据它写出C代码即可。

主要就是在页错误异常处理程序中，当`mm`结构体拥有此虚拟地址但页表中还没有映射时，调用`pgdir_alloc_page`来分配并映射一个物理页面。这其实是按需分配的实现。

页目录项和页表项的字段在Lab 2实验报告中已做说明，这里重点说明对实现页替换算法有重要作用的字段。

主要使用页表项的字段。页表项的字段和页目录项十分类似：

```
 31               12 11 9 8 7 6 5 4 3 2 1 0
+-------------------+----+-+-+-+-+-+-+-+-+-+
|   页基地址高20位   |忽略|G|0|D|A|C|T|U|W|P|
+-------------------+----+-+-+-+-+-+-+-+-+-+
```

与页替换算法有关的字段有：

- A：在上次清零之后，该页是否被访问（读或者写）过，可用于时钟替换算法和扩展时钟替换算法的实现
- D：在上次清零之后，该页是否被写过，可用于扩展时钟替换算法的实现
- P：页面是否存在于内存中，当此位清零时，其余位都可供内核自由使用。这可以用来存放交换分区的一些信息，可用于各种页替换算法的实现。

如果ucore的页错误处理程序执行过程中访问内存，又出现了页错误异常，由于这个异常不可以屏蔽，处理器会将异常的线性地址保存在CR2寄存器中，然后查询IDT找到中断服务程序入口点。由于当前已在内核态，不涉及特权级的变化，处理器还会向当前栈中压入cs、eip和错误码。错误码记录了异常的一些标志，比如是读还是写操作触发了异常，是非法访问还是缺页触发了异常，这对于页替换算法和写时复制的实现都有用。最后，跳转至中断服务程序。这些事情中间还会做权限检查，不通过还会触发保护错误。这些事情执行的顺序可能与处理器实现有关。

与参考答案对比，我缺少了对于出错情况检查的代码，应修复。

## 练习2

这个练习提供的注释已经写得非常详细，比实际要写的代码还要多，根据它写出C代码即可。

在成功分配一个物理页面的时候将此页面插入物理页面链表尾；当需要选择一个页面被换出时，选择链表头的页面返回。这就实现了先进先出替换算法。

此外，若某页面已在页表中映射（页表项为非零值）但还是产生了缺页异常，说明此页面被换出了。此时，缺页异常处理程序需要选择一个页面换出，然后将触发缺页异常的正在被访问的页面换入并修改页表映射。还要设置好`Page`结构体的`pra_vaddr`，此字段说明了该物理页面对应的虚拟地址，在换出时用于计算该页面应写入交换分区的位置。

与参考答案对比，我缺少了对于出错情况检查的代码，应修复。

扩展时钟替换算法设计与实现见下。

## 扩展练习 - 扩展时钟替换算法

这个扩展练习的[实现](https://github.com/twd2/ucore_os_lab/compare/master...twd2:lab3-challenge)在GitHub [lab3-challenge分支](https://github.com/twd2/ucore_os_lab/tree/lab3-challenge/labcodes/lab3)中，文件主要为`swap_extclk_dirty.c`。

算法的实现完全按照MOOC中对于扩展时钟替换算法的讲解，不过有以下实现上的细节。

我的设计中，这个替换算法在运行时有两个状态：启动状态和工作状态。算法主要维护两个数据：物理页面链表以及扩展时钟替换算法的时钟指针。

在初始化之后，算法处于启动状态，这时候算法对于系统可用的物理页面还不了解，当`map_swappable`被调用，且物理页面还不在物理页面链表中，说明有一个新的物理页面被分配，算法将其放入物理页面链表中。

`swap_out_victim`被第一次调用时，说明系统物理内存均已分配完毕，也说明物理页面链表中已包含了全部的物理页面，可以转入工作状态真正开始运行替换算法，选择页面换出了。

在选择页面换出时，算法以时钟指针为起始，不断遍历这个物理页面链表，到链表尾部则回到头部继续遍历。物理页面链表其实就是一个循环链表，方便了实现。对于每一个被遍历的物理页面，考察它对应的虚拟地址的页表项中AD两个位（AD两位说明见上）：

1. 若A位为0，D位也为0，则选择这个页面被换出，将时钟指针指向链表下一项，本次算法结束
2. 若A位为0，D位为1，则将此页面写入交换分区，写入成功后清除D位，并刷新TLB
3. 若A位为1，则将其清零并刷新TLB，不考虑D位的情况

刷新TLB是为了让页面再次被访问或被修改时，AD位能够再次被处理器置位。若不清除，处理器根据TLB的缓存会认为AD位已经被置位，而不修改内存中的值，算法在下次读取时将读到错误的值。这确实会导致性能的损失，但确保了算法和MOOC的讲解一致。

为了减少写交换分区的次数，优化性能，还需修改`swap_out`函数，在判断D位已被清零的情况下不再写入交换分区。

此外，实际使用该算法时，在清除D位并将页面写入交换分区时，还可以采用磁盘队列，将写入操作放在后台执行，降低延迟。

经测试，我的实现用MOOC上的测例运行（见`check_swap`），缺页次数、缺页发生的时刻以及每次缺页时算法执行情况都和MOOC上一致，说明实现大致正确。

同时，我也实现了不考虑D位的时钟替换算法，文件主要为`swap_extclk.c`。与考虑D位的区别在于，对于每一个被遍历的物理页面，只考察它对应的虚拟地址的页表项中A位（A位说明见上）：

1. 若A位为0，则选择这个页面被换出，将时钟指针指向链表下一项，本次算法结束
2. 若A位为1，则将其清零并刷新TLB

总之，算法在AD位都为0（对于不考虑D位的算法，则A位为0就够了）时会真正选择页面换出，通过查询页表可以得到AD位的具体信息，在发生页错误异常并且权限检查通过后会执行页面替换算法来进行换入和换出操作。

与参考答案对比，我认为我的方法思路清晰、实现简洁。
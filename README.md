# 项目简介

## 日志管理

``` text
Log4J

Logger (日志器)
|
|-------Formatter（日志格式）
|
Appender（日志输出器）
```


## 配置系统

``` text
Config: Yaml-config
```

yaml-cpp

``` shell
# use package-manager-tool
# package-manager --install yaml-cpp

# git
# git-clone yaml-cpp
mkdir build && cd build && cmake .. && make install
```

## 线程配置

Thread, Mutex -> pthread

互斥量 mutex， 信号量 mutex

Spinlock 替换 Mutex

写文件，周期 reopen，避免文件被删除


## 协程封装

协程的接口

```
CREATE:
Thread->main_fiber ------------------->  sub_fiber
           |               |
        (unique)     (new to create)


OPERATION:
Thread->main_fiber ------------------>  sub_fiber
                          |
                      (swapin)


Thread->main_fiber <------------------  sub_fiber
                          |
                      (swapout)
```

协程调度模块 schedule
```
       +------------------------+
       |        Scheduler       |           调度中心
       |------------------------|
       |    - 线程池            |
       |    - 协程队列          |
       |    - 各种管理函数      |
       +------------------------+
             /           \
            /             \
           V      ...      V
    +------------+   +------------+
    | Thread A   |   | Thread B   |      工作线程（创建自 Scheduler）
    |------------|   |------------|
    | Fiber main |   | Fiber main |      每个线程先有个主协程
    | Fiber X    |   | Fiber Y    |      被调度运行的任务协程
    +------------+   +------------+

1. 线程池，分配一组线程
2. 协程调度器，将协程指定到相应的线程上执行

schedule(fiber/func)
start() / stop() / run()

1. 设置当前线程的scheduler
2. 设置当前线程的 run, fiber
3. 协程调度循环(while)
   1. 协程调度里面是否存在任务
   2. 无任务执行，那么就执行 idle
```

生命周期与状态转移
```
[INIT]
   |
   |  swapIn()（第一次）
   v
[EXEC]
   | \
   |  \------------------> [EXCEPT]
   |              执行出错 ↑
   |
   |  正常执行完
   |---------------------> [TERM]
   |
   |  调用 Fiber::YieldToReady()
   |---------------------> [READY]
   |
   |  调用 Fiber::YieldToHold()
   |---------------------> [HOLD]

```


run的基本形式
```
调度器的 run()
while (true) {
    1. 从任务队列 m_fibers 中取出一个任务（ft）；

    2. 如果是协程（ft.fiber）：
        - 如果状态合法，就执行 swapIn 切换；
        - 执行完后检查状态（是否终止、yield、挂起）；
        - 如果是 READY，重新调度；否则 HOLD。

    3. 如果是普通函数（ft.cb）：
        - 包装成协程对象执行；
        - 同样根据状态做处理。

    4. 如果啥也没有：
        - 当前线程运行一个 idle 协程，做“空转”处理；

    5. 每轮结束后 reset 清理 ft；
}
```

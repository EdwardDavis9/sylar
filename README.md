# 项目简介

## 日志管理

``` text
LoggerManager (日志管理器)
|
|-- Logger (日志器)
|     |
|     |-- LogFormatter (日志格式器)
|     |
|     |-- LogAppender (日志输出器, 抽象基类)
|             |
|             |-- StdoutLogAppender (输出到控制台)
|             |
|             |-- FileLogAppender (输出到文件)
|
|-- LogLevel (日志级别)
|
|-- LogEvent (日志事件, 保存日志数据)
|
|-- LogEventWrap (事件包装器, RAII, 保证日志能正确写出)
```

## 配置系统

``` text
Config: Yaml-config

|---------------------------|
|    config  |   (YAML)     |
|---------------------------|
             | Load
             V
|---------------------------|
|       YAML::Node parser   |
|---------------------------|
             |  (LexicalCast)
             V
|---------------------------|
|     ConfigVar<T> instance |
|---------------------------|
             | Store in the global table
             V
|---------------------------|
|      Config::GetDatas()   |
|---------------------------|
```

yaml-cpp

``` shell
# use package-manager-tool
# package-manager --install yaml-cpp

# git
# git-clone yaml-cpp
mkdir build && cd build && cmake .. && make install
```

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
``` text
       +------------------------+
       |        Scheduler       |           调度中心
       |------------------------|
       |    - thread pool       |
       |    - fiber queue       |
       |    - xxxFunc           |
       +------------------------+
             /           \
            /             \
           V      ...      V
    +------------+   +------------+
    | Thread A   |   | Thread B   |      工作线程(创建自 Scheduler)
    |------------|   |------------|
    | Fiber main |   | Fiber main |      每个线程先有个主协程
    | Fiber X    |   | Fiber Y    |      被调度运行的任务协程
    +------------+   +------------+

```

1. 线程池, 分配一组线程
2. 协程调度器, 将协程指定到相应的线程上执行

schedule(fiber/func)
start() / stop() / run()

1. 设置当前线程的scheduler
2. 设置当前线程的 run, fiber
3. 协程调度循环(while)
   1. 协程调度里面是否存在任务
   2. 无任务执行, 那么就执行 idle

生命周期与状态转移

``` text
[INIT]
   |
   |  swapIn()(first)
   v
[EXEC]
   | \
   |  \------------------> [EXCEPT]
   |              error      ↑
   |
   |  Executed successfully
   |
   |---------------------> [TERM]
   |
   |  call Fiber::YieldToReady()
   |
   |---------------------> [READY]
   |
   |  call Fiber::YieldToHold()
   |
   |---------------------> [HOLD]

```


run的基本形式

``` text
调度器的 run()
while (true) {
    1. 从任务队列 m_fibers 中取出一个任务(ft);

    2. 如果是协程(ft.fiber):
        - 如果状态合法, 就执行 swapIn 切换;
        - 执行完后检查状态(是否终止, yield, 挂起);
        - 如果是 READY, 重新调度; 否则 HOLD.

    3. 如果是普通函数(ft.cb):
        - 包装成协程对象执行;
        - 同样根据状态做处理.

    4. 如果没有任务:
        - 当前线程运行一个 idle 协程, 做"空转"处理;

    5. 每轮结束后 reset 清理 ft;
}
```


Timer -> addTimer() -> cancel()
获取当前定时器触发离现在的时间差
返回当前需要触发的定时器


```
  [Fiber]            [Timer]
     ^                  ^
     |                  |
     |                  |
     |                  |
     v       .          v
  [Thread]        [TimerManager]
     ^                  ^
     |                  |
     |                  |
     |                  |
     v       .          v
  [Schedule]        [IOManager]
```


## 线程配置

Thread, Mutex -> pthread

互斥量 mutex,  信号量 mutex

Spinlock 替换 Mutex

写文件, 周期 reopen, 避免文件被删除

## Socket 配置

```
       +------------------------+
       |         Address        |
       +------------------------+
                   |
                   |
                   V
             +------------+
             |  IPAddress |
             +------------+
             /            \
            /              \
           V       ...      V
    +-------------+   +-------------+
    | IPv4Address |   | IPv6Address |
    +-------------+   +-------------+

```

## ByteArray 序列化配置

=ByteArray= 是用于 **二进制序列化 / 反序列化** 的核心组件.
它提供了一种 **基于内存块(Node)链表** 的可变长度字节缓冲区,
支持 **多种整数类型, 浮点类型, 字符串的读写**,
并可选择性支持 **压缩存储(Varint 编码)** 以节省空间.


=ByteArray= 的底层不是简单的 =std::vector<char>=,
而是一个 **由 Node 组成的链表结构**:

```
+--------+     +--------+     +--------+
| Node 1 | --> | Node 2 | --> | Node 3 | --> ...
+--------+     +--------+     +--------+
```

每个 =Node= 拥有一段固定大小的内存(默认 4KB),
整个 =ByteArray= 就是这些节点的逻辑拼接.

当空间不够时:

> 自动分配新 Node 并挂接到链表末尾.
> **"用链表式内存块模拟无限大的顺序缓冲区."**



## TCP server 封装

基于 TCP server 实现一个 EchoServer

## Stream 针对 File/Socket 封装


## 压测

``` shell
ab -n 1000000 -c 200 # 发送 100 万次请求, 200 的并发
```

## 压测效果

### 本服务器程序
``` shell
# 单线程
➜  sylar git:(main) ✗ ab -n 1000000 -c 200 "127.0.0.1:8020/sylar"
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        sylar/1.0.0
Server Hostname:        127.0.0.1
Server Port:            8020

Document Path:          /sylar
Document Length:        138 bytes

Concurrency Level:      200
Time taken for tests:   43.425 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    23028.11 [#/sec] (mean)
Time per request:       8.685 [ms] (mean)
Time per request:       0.043 [ms] (mean, across all concurrent requests)
Transfer rate:          5622.10 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    4   0.7      4      11
Processing:     1    5   1.0      5      15
Waiting:        0    4   1.0      4      14
Total:          5    9   1.1      8      20

Percentage of the requests served within a certain time (ms)
  50%      8
  66%      9
  75%      9
  80%      9
  90%     10
  95%     11
  98%     13
  99%     14
 100%     20 (longest request)

# 2 线程
➜  sylar git:(main) ✗ ab -n 1000000 -c 200 "127.0.0.1:8020/sylar"
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        sylar/1.0.0
Server Hostname:        127.0.0.1
Server Port:            8020

Document Path:          /sylar
Document Length:        138 bytes

Concurrency Level:      200
Time taken for tests:   48.349 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    20682.90 [#/sec] (mean)
Time per request:       9.670 [ms] (mean)
Time per request:       0.048 [ms] (mean, across all concurrent requests)
Transfer rate:          5049.54 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    4   1.1      4      13
Processing:     2    5   1.2      5      23
Waiting:        0    4   1.1      4      22
Total:          6   10   1.6      9      25

Percentage of the requests served within a certain time (ms)
  50%      9
  66%     10
  75%     11
  80%     11
  90%     12
  95%     13
  98%     14
  99%     14
 100%     25 (longest request)

# 3 线程
➜  sylar git:(main) ✗ ab -n 1000000 -c 200 "127.0.0.1:8020/sylar"
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        sylar/1.0.0
Server Hostname:        127.0.0.1
Server Port:            8020

Document Path:          /sylar
Document Length:        138 bytes

Concurrency Level:      200
Time taken for tests:   48.822 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    20482.51 [#/sec] (mean)
Time per request:       9.764 [ms] (mean)
Time per request:       0.049 [ms] (mean, across all concurrent requests)
Transfer rate:          5000.61 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    5   1.1      4      12
Processing:     2    5   1.1      5      21
Waiting:        0    4   0.9      3      18
Total:          6   10   1.5      9      23
WARNING: The median and mean for the waiting time are not within a normal deviation
        These results are probably not that reliable.

Percentage of the requests served within a certain time (ms)
  50%      9
  66%     10
  75%     11
  80%     11
  90%     12
  95%     13
  98%     13
  99%     13
 100%     23 (longest request)
```



### nginx

``` shell
# 似乎是单线程的情况
 ~/Code/cc/sylar/ [main*] ab -n 1000000 -c 200 "127.0.0.1:80/sylar"
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 100000 requests
Completed 200000 requests
Completed 300000 requests
Completed 400000 requests
Completed 500000 requests
Completed 600000 requests
Completed 700000 requests
Completed 800000 requests
Completed 900000 requests
Completed 1000000 requests
Finished 1000000 requests


Server Software:        nginx/1.28.0
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /sylar
Document Length:        153 bytes

Concurrency Level:      200
Time taken for tests:   46.814 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      303000000 bytes
HTML transferred:       153000000 bytes
Requests per second:    21361.09 [#/sec] (mean)
Time per request:       9.363 [ms] (mean)
Time per request:       0.047 [ms] (mean, across all concurrent requests)
Transfer rate:          6320.71 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    4   0.8      4      12
Processing:     2    6   1.1      5      15
Waiting:        0    4   1.2      4      12
Total:          4    9   1.3      9      19

Percentage of the requests served within a certain time (ms)
  50%      9
  66%      9
  75%     10
  80%     10
  90%     11
  95%     12
  98%     13
  99%     13
 100%     19 (longest request)
```


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
# mkdir build && cd build && cmake .. && make install
```


## 线程配置

线程模块封装的是线程这一物理执行单位,以及资源同步相关的操作

Thread 是对 pthread 的 RAII 封装,代表一个物理执行线程.构造函数传入回调函数和线程名称,立即启动线程并通过信号量同步等待初始化完成,保证调用者拿到对象时线程已就绪.
线程局部存储 (t_thread, t_thread_name) 配合静态方法提供线程身份查询;join() 等待线程结束,析构时若未 join 则自动 detach,防止资源泄漏

| 类型                        | 底层                 | 作用                                                           |
|-----------------------------|----------------------|----------------------------------------------------------------|
| `Semaphore`                 | `sem_t`              | 信号量,控制线程并发数量或实现线程间通知与等待                 |
| `Mutex`                     | `pthread_mutex_t`    | 互斥锁,保护临界区,保证共享资源的互斥访问                     |
| `RWMutex`                   | `pthread_rwlock_t`   | 读写锁,读共享,写独占,适合读多写少场景                       |
| `Spinlock`                  | `pthread_spinlock_t` | POSIX 库提供的自旋锁,内部有退避优化;短时忙等,适合极短临界区 |
| `CASLock`                   | `std::atomic_flag`   | 纯用户态自旋锁,通过 CAS 原子操作实现,适合更轻量的临界区      |
| `NullMutex` / `NullRWMutex` | 空操作               | 用于无需同步的模板场景,消除锁开销                             |

所有锁类型都提供了对应的 RAII 包装(如 `ScopedLockImpl`),使用时声明 `Mutex::Lock lock(mutex);` 即可在作用域结束时自动释放.

## 协程和调度器封装

协程的主流程

```
main_fiber             idle_fiber                task_fiber
     |                           |                         |
     +---- 队列空, swapIn ------->|                         |
     |                           +-- 执行 idle()            |
     |                           |   (epoll_wait)          |
     |                           +-- 事件触发后 YieldToHold  |
     | <-------- swapOut --------+                         |
     | 继续循环,发现新任务                                    |
     +---------------------------------- swapIn ---------->|
     |                                                     +-- 执行用户代码
     |                                                     +-- 结束,自动 swapOut
     | <-------------------------------- swapOut ----------+
     | 继续循环 ...
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
start()/stop()/run()

1. 设置当前线程的scheduler
2. 线程将 run 作为线程任务函数执行, 调度并执行共享任务资源
3. 协程调度循环(while)
   1. 协程调度里面是否存在任务
   2. 无任务执行, 那么就执行 idle

生命周期与状态转移

``` text
INIT   -- swapIn         --> EXEC
EXEC   -- 正常结束        --> TERM
       -- 异常            --> EXCEPT
       -- YieldToReady() --> READY
       -- YieldToHold()  --> HOLD

READY  -- swapIn --> EXEC
HOLD   -- swapIn --> EXEC

TERM   -- reset() --> INIT
EXCEPT -- reset() --> INIT
```


run的基本形式

``` text
调度器的 run()
while (true) {
    1. 从任务队列 m_fibers 中取出一个任务(ft);

    2. 如果是协程(ft.fiber):
        - 如果状态合法, 就执行 swapIn 切换;
        - 执行完后检查状态(是否终止, yield, 挂起);
        - 如果是 READY, 那么重新调度; 否则 HOLD.

    3. 如果是普通函数(ft.cb):
        - 包装成协程对象执行;
        - 同样根据状态做处理.

    4. 如果没有任务:
        - 当前线程运行一个 idle 协程, 进行 idle 处理;
}
```

TimerManager 封装的是定时事件相关的操作,通过时间整型值和回调函数实现定时器的效果.维护按时间排序的定时器集合,提供添加,取消,刷新,到期时提取回调列表

IOManager 是融合了 epoll IO 多路复用与定时器管理的协程调度器.它将每个 fd 的读写事件与一个协程(或回调)绑定,当事件就绪时,将该协程重新放入调度队列;同时它利用定时器的定时时间来驱动 epoll_wait 的超时,这样就能将事件对应的 fd 上协程任务重新放回调度队列

## Socket 配置

Address 模块封装的是和地址相关的内容,例如 sockaddr 以及 sockaddrlen_t 等相关的内容

```
       +------------------------+
       |         Address        |
       +------------------------+
                   ^
                   |
                   |
    +------------------------------+
    |          IPAddress           |
    +------------------------------+
          ^       ^         ^     ^
         /        |         |      \
        /         |         |       \
    +------+   +------+  +------+  +--------+
    | IPv4 |   | IPv6 |  | Unix |  | UnKnow |
    +------+   +------+  +------+  +--------+
```

fd_manager 模块封装的是文件描述符相关的内容,例如阻塞状态, 超时时间, socket 套接字标识之类的

socket 模块封装的是和网络套接字相关的内容,例如 socket, bind, listen, connect 等类似的操作

## ByteArray 序列化配置

**ByteArray** 是用于 **二进制序列化 / 反序列化** 的核心组件.

它提供了一种 **基于内存块(Node)链表** 的可变长度字节缓冲区,
支持 **多种整数类型, 浮点类型, 字符串的读写**,
并可选择性支持 **压缩存储(Varint 编码)** 以节省空间.


**ByteArray** 的底层不是简单的 **std::vector<char>**,而是一个 **由 Node 组成的链表结构**:

```
+--------+     +--------+     +--------+
| Node 1 | --> | Node 2 | --> | Node 3 | --> ...
+--------+     +--------+     +--------+
```

每个 **Node** 拥有一段固定大小的内存(默认 4KB),
整个 **ByteArray** 就是这些节点的逻辑拼接.

当空间不够时:

> 自动分配新 Node 并挂接到链表末尾. **"用链表式内存块模拟无限大的顺序缓冲区."**


## TCP server 封装

http 模块封装的是请求和响应相关的内容, 即把请求和响应抽象成对象进行操作

http_parser 模块封装的是解析相关的内容,即请求报文解析和响应报文解析,通过 ragel 生成状态机进行报文的解析和回调赋值

项目中并未提供 ragel 生成的代码文件，因此需要手动安装 ragel 才可以正常编译
``` bash
# package-manager install ragel
```
tcp_server 模块封装是服务器的上层应用行为, 也是实现主从 reactor 的关键, 通过 acceptWorker 获取连接对象,然后将连接对象交给 worker 去回调 handleClient 函数

http_server 模块封装的是关于 http 相关的行为,例如通过 servlet 对象去匹配 parser 解析到的请求路径,然会回调对应的函数,返回不同的消息体,从而实现路由转发,以及 tcp_server 中的 handleClient 函数,实现了和客户端的通信的具体行为


## 压测

``` shell
ab -n 1000000 -c 1024    # 发送 100 万次请求, 1024 的并发的短连接请求
ab -n 1000000 -c 1024 -k # 发送 100 万次请求, 1024 的并发的长连接请求
```

### 服务器程序(单 reactor 单线程/进程)
``` shell
# 1 百万请求, 1000 并发, 短连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 http://127.0.0.1:8020/sylar
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

Concurrency Level:      1024
Time taken for tests:   44.306 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    22570.18 [#/sec] (mean)
Time per request:       45.370 [ms] (mean)
Time per request:       0.044 [ms] (mean, across all concurrent requests)
Transfer rate:          5510.30 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   19   2.3     19      34
Processing:     9   26   3.9     28      54
Waiting:        0   20   4.7     23      49
Total:         22   45   2.9     46      72

Percentage of the requests served within a certain time (ms)
  50%     46
  66%     46
  75%     46
  80%     46
  90%     47
  95%     51
  98%     56
  99%     56
 100%     72 (longest request)


# 1 百万请求, 1000 并发, 长连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 -k http://127.0.0.1:8020/sylar
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

Concurrency Level:      1024
Time taken for tests:   16.112 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    1000000
Total transferred:      255000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    62065.58 [#/sec] (mean)
Time per request:       16.499 [ms] (mean)
Time per request:       0.016 [ms] (mean, across all concurrent requests)
Transfer rate:          15455.78 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.8      0      38
Processing:     0   16   1.2     16      38
Waiting:        0   16   1.2     16      31
Total:          0   16   1.3     16      48

Percentage of the requests served within a certain time (ms)
  50%     16
  66%     16
  75%     16
  80%     16
  90%     17
  95%     17
  98%     17
  99%     22
 100%     48 (longest request)
```

### 服务器程序(多 reactor, 主从 reactor 多线程/进程)
``` shell

# 1 百万请求, 1000 并发, 短连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 http://127.0.0.1:8020/sylar
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

Concurrency Level:      1024
Time taken for tests:   49.237 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    20310.06 [#/sec] (mean)
Time per request:       50.418 [ms] (mean)
Time per request:       0.049 [ms] (mean, across all concurrent requests)
Transfer rate:          4958.51 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   25   4.1     24      45
Processing:    10   26   4.2     25      48
Waiting:        0   19   3.2     18      40
Total:         23   50   5.0     49      73

Percentage of the requests served within a certain time (ms)
  50%     49
  66%     52
  75%     54
  80%     55
  90%     57
  95%     60
  98%     63
  99%     64
 100%     73 (longest request)


# 1 百万请求, 1000 并发, 长连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 -k http://127.0.0.1:8020/sylar
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

Concurrency Level:      1024
Time taken for tests:   7.136 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    1000000
Total transferred:      255000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    140135.81 [#/sec] (mean)
Time per request:       7.307 [ms] (mean)
Time per request:       0.007 [ms] (mean, across all concurrent requests)
Transfer rate:          34897.10 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.7      0      28
Processing:     0    7   1.4      7      28
Waiting:        0    7   1.4      7      20
Total:          0    7   1.5      7      37

Percentage of the requests served within a certain time (ms)
  50%      7
  66%      7
  75%      7
  80%      7
  90%     10
  95%     10
  98%     11
  99%     11
 100%     37 (longest request)
```

### nginx(使用 1 个进程）

``` shell
#短连接
~/Code/cc/sylar>>ab -n 1000000 -c 1024 http://127.0.0.1/sylar/
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


Server Software:        nginx/1.30.0
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /sylar/
Document Length:        138 bytes

Concurrency Level:      1024
Time taken for tests:   44.358 seconds
Complete requests:      1000000
Failed requests:        0
Total transferred:      282000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    22544.08 [#/sec] (mean)
Time per request:       45.422 [ms] (mean)
Time per request:       0.044 [ms] (mean, across all concurrent requests)
Transfer rate:          6208.43 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   21   2.5     21      33
Processing:     9   25   3.9     24      44
Waiting:        0   18   4.1     16      35
Total:         27   45   2.9     44      67

Percentage of the requests served within a certain time (ms)
  50%     44
  66%     45
  75%     47
  80%     47
  90%     48
  95%     51
  98%     55
  99%     57
 100%     67 (longest request)

# 长连接
~/Code/cc/sylar>>ab -n 1000000 -c 1024 -k http://127.0.0.1/sylar/
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


Server Software:        nginx/1.30.0
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /sylar/
Document Length:        138 bytes

Concurrency Level:      1024
Time taken for tests:   7.562 seconds
Complete requests:      1000000
Failed requests:        0
Keep-Alive requests:    1000000
Total transferred:      287000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    132237.05 [#/sec] (mean)
Time per request:       7.744 [ms] (mean)
Time per request:       0.008 [ms] (mean, across all concurrent requests)
Transfer rate:          37062.53 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0      24
Processing:     4    8   0.2      8      24
Waiting:        0    8   0.2      8      12
Total:          4    8   0.6      8      33

Percentage of the requests served within a certain time (ms)
  50%      8
  66%      8
  75%      8
  80%      8
  90%      8
  95%      8
  98%      8
  99%      8
 100%     33 (longest request)
```

### nginx(使用 4 个进程)

``` shell
# 短连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 http://127.0.0.1/sylar/
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


Server Software:        nginx/1.30.0
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /sylar/
Document Length:        138 bytes

Concurrency Level:      1024
Time taken for tests:   44.941 seconds
Complete requests:      1000000
Failed requests:        0
Total transferred:      282000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    22251.33 [#/sec] (mean)
Time per request:       46.020 [ms] (mean)
Time per request:       0.045 [ms] (mean, across all concurrent requests)
Transfer rate:          6127.81 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   21   2.5     21      45
Processing:     9   25   4.0     24      46
Waiting:        0   18   4.2     16      35
Total:         27   46   3.3     44      65

Percentage of the requests served within a certain time (ms)
  50%     44
  66%     46
  75%     48
  80%     48
  90%     49
  95%     53
  98%     57
  99%     59
 100%     65 (longest request)

# 长连接压测
~/Code/cc/sylar>>ab -n 1000000 -c 1024 -k http://127.0.0.1/sylar/
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


Server Software:        nginx/1.30.0
Server Hostname:        127.0.0.1
Server Port:            80

Document Path:          /sylar/
Document Length:        138 bytes

Concurrency Level:      1024
Time taken for tests:   6.079 seconds
Complete requests:      1000000
Failed requests:        0
Keep-Alive requests:    999766
Total transferred:      286998830 bytes
HTML transferred:       138000000 bytes
Requests per second:    164489.46 [#/sec] (mean)
Time per request:       6.225 [ms] (mean)
Time per request:       0.006 [ms] (mean, across all concurrent requests)
Transfer rate:          46101.84 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      29
Processing:     0    6   3.4      8      29
Waiting:        0    6   3.4      8      15
Total:          0    6   3.5      8      39

Percentage of the requests served within a certain time (ms)
  50%      8
  66%      8
  75%      8
  80%      8
  90%      8
  95%      8
  98%      9
  99%     12
 100%     39 (longest request)
```

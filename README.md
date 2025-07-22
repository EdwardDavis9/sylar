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
Thread->main_fiber <--------->  sub_fiber
```


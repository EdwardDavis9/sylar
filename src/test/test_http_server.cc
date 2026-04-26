#include "http/http_server.hh"
#include "sylar/iomanager.hh"
#include "sylar/log.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// ========== 测试配置 ==========
// 1: 单 reactro 单进程/线程  2: 多 reactro(主从 reactor) 多进程/线程
#define CONFIG_MODE 1
#define WORKER_THREADS 4
// 工作线程数，仅在 CONFIG_MODE == 2 时生效
// =============================

void run(sylar::IOManager *accept_worker, sylar::IOManager *worker) {
    g_logger->setLevel(sylar::LogLevel::INFO);

#if CONFIG_MODE == 1
    // 单 reactro 单进程/线程
    sylar::http::HttpServer::ptr server(new sylar::http::HttpServer(true));
#elif CONFIG_MODE == 2
    // 多 reactro(主从 reactor) 多进程/线程
    sylar::http::HttpServer::ptr server(
        new sylar::http::HttpServer(true, worker, accept_worker));
#endif

    // 绑定端口
    sylar::Address::ptr addr =
        sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(1);
    }

    // 注册 servlet（保持不变）
    auto sd = server->getServletDispatcher();
    sd->addServlet("/sylar/xx", [](sylar::http::HttpRequest::ptr req,
                                   sylar::http::HttpResponse::ptr rsp,
                                   sylar::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/sylar/*", [](sylar::http::HttpRequest::ptr req,
                                      sylar::http::HttpResponse::ptr rsp,
                                      sylar::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    server->start();
}

int main() {
#if CONFIG_MODE == 1
    // 单 reactor 单线程/进程模型
    sylar::IOManager iom(1, false, "main");
    iom.schedule([&]() { run(&iom, &iom); });

#elif CONFIG_MODE == 2
    // 多 reactor(主从 reactor) 多线程/进程模型
    sylar::IOManager accept_iom(1, false, "accept"); // 单线程 accept
    sylar::IOManager worker_iom(WORKER_THREADS, false,
                                "worker"); // 多线程 worker

    accept_iom.schedule([&]() {
        run(&accept_iom, &worker_iom);
        // run(&accept_iom, &accept_iom);
    });

    // 主线程等待（简单用 pause，Ctrl+C 终止）
    pause();
#endif

    return 0;
}

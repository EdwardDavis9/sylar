#include "http/http_server.hh"
#include "sylar/log.hh"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

#define to_string(...) #__VA_ARGS__

// sylar::IOManager::ptr woker;

void run()
{
    g_logger->setLevel(sylar::LogLevel::INFO);

    sylar::http::HttpServer::ptr server(new sylar::http::HttpServer(true));

    sylar::Address::ptr addr =
        sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");

    while (!server->bind(addr)) {
        sleep(2);
    }

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

    sd->addGlobServlet("/sylarx/*", [](sylar::http::HttpRequest::ptr req
                ,sylar::http::HttpResponse::ptr rsp
                ,sylar::http::HttpSession::ptr session) {
            rsp->setBody(to_string(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });


    server->start();
}

int main(int argc, char *argv[])
{
    // sylar::IOManager iom(2);
    sylar::IOManager iom(4);
    // sylar::IOManager iom(1, true, "main");

    // woker.reset(new sylar::IOManager(3, false, "worker"));

    iom.schedule(run);

    return 0;
}

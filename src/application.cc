#include "sylar/application.hh"
#include "sylar/log.hh"
#include "sylar/env.hh"
#include "sylar/config.hh"
#include "sylar/daemon.hh"
#include "sylar/config.hh"
#include "sylar/address.hh"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<std::string>::ptr g_server_work_path =
    sylar::Config::Lookup("server.work_path", std::string("/apps/work/sylar"),
                          "server work path");

static sylar::ConfigVar<std::string>::ptr g_server_pid_file =
    sylar::Config::Lookup("server.pid_file", std::string("sylar.pid"),
                          "server pid file");

struct HttpServerConf {
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout   = 1000 * 2 * 60;
    std::string name;

    bool isVaild() const { return !address.empty(); }

    bool operator==(const HttpServerConf &oth) const
    {
        return address == oth.address && keepalive == oth.keepalive
               && name == oth.name;
    }
};

template <>
class LexicalCast<std::string, HttpServerConf> {
  public:
    HttpServerConf operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout   = node["timeout"].as<int>(conf.timeout);
        conf.name      = node["name"].as<std::string>(conf.name);
        if (node["address"].IsDefined()) {
            for (size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template <>
class LexicalCast<HttpServerConf, std::string> {
  public:
    std::string operator()(const HttpServerConf &conf)
    {
        YAML::Node node;
        node["name"]      = conf.name;
        node["timeout"]   = conf.timeout;
        node["keepalive"] = conf.keepalive;
        for (auto &i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static sylar::ConfigVar<std::vector<HttpServerConf>>::ptr g_http_server_conf =
    sylar::Config::Lookup("http_servers", std::vector<HttpServerConf>(),
                          "http server config");

Application *Application::s_instance = nullptr;
Application::Application() { s_instance = this; }

auto Application::init(int argc, char **argv) -> bool
{
    m_argc = argc;
    m_argv = argv;

    // 添加帮组信息
    sylar::EnvMgr::GetInstance()->addHelp("s", "start writh the terminal");
    sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    sylar::EnvMgr::GetInstance()->addHelp("c", "conf path default: ../conf");
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");

    if (!sylar::EnvMgr::GetInstance()->init(argc, argv)) {
        sylar::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    if (sylar::EnvMgr::GetInstance()->has("p")) {
        sylar::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    uint8_t run_type = 0;
    if (sylar::EnvMgr::GetInstance()->has("s")) {
        run_type = 1;
    }

    if (sylar::EnvMgr::GetInstance()->has("d")) {
        run_type = 2;
    }

    // SYLAR_LOG_INFO(g_logger) << "run_type: " << run_type ;

    if (run_type == 0) {
        // sylar::EnvMgr::GetInstance()->printHelp();
        return false;
    }


    // 默认的配置路径位于: bin/conf/
    // 而默认的程序路径位于: bin/test/, 因此需要先返回上一级目录
    std::string conf_path = sylar::EnvMgr::GetInstance()->getAbsolutePath(
        sylar::EnvMgr::GetInstance()->get("c", "../conf"));

    SYLAR_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    sylar::Config::LoadFromConfDir(conf_path); // 加载配置文件

    std::string pidfile =
        g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

    // 确保服务还未运行
    if (sylar::FSUtil::ISRunningPidFile(pidfile)) {
        SYLAR_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }

    if (!sylar::FSUtil::MKDir(g_server_work_path->getValue())) {
        SYLAR_LOG_ERROR(g_logger)
            << "create work path [" << g_server_work_path->getValue()
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

auto Application::run() -> bool
{
    bool is_daemon = sylar::EnvMgr::GetInstance()->has("d");
    return start_daemon(
        m_argc, m_argv,
        [this](int argc, char **argv) { return main(argc, argv); }, is_daemon);
}

auto Application::main(int argc, char **argv) -> int
{
    SYLAR_LOG_INFO(g_logger) << "main";
    {
        // 创建一个 pidfile
        std::string pidfile = g_server_work_path->getValue() + "/"
                              + g_server_pid_file->getValue();

        std::ofstream ofs(pidfile);
        if (!ofs) {
            SYLAR_LOG_ERROR(g_logger)
                << "open pidfile " << pidfile << " failed";
            return false;
        }
        ofs << getpid();
    }

    sylar::IOManager iom(1);
    iom.schedule([this]() { this->run_fiber(); });
    iom.stop(); // 在停止前,主动去执行这个任务

    return 0;
}

auto Application::run_fiber() -> int
{
    // 一般情况下,init 函数中已经加载了配置,这里是从配置中获取信息
    auto http_conf = g_http_server_conf->getValue();
    for (auto &i : http_conf) {
        SYLAR_LOG_INFO(g_logger)
            << LexicalCast<HttpServerConf, std::string>()(i);

        std::vector<Address::ptr> address;
        // 收集监听目标
        for (auto &a : i.address) {
            size_t pos = a.find(":");

            // 是 Unix 本地地址的话
            if (pos == std::string::npos) {
                address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                continue;
            }

            // 是网络地址的话
            int32_t port = atoi(a.substr(pos + 1).c_str());
            auto addr =
                sylar::IPAddress::Create(a.substr(0, pos).c_str(), port);
            if (addr) {
                address.push_back(addr);
                continue;
            }

            // 是网卡地址的话
            std::vector<std::pair<Address::ptr, uint32_t>> result;
            if (!sylar::Address::GetInterFaceAddresses(result,
                                                       a.substr(0, pos)))
            {
                SYLAR_LOG_ERROR(g_logger) << "invaild address: " << a;
                continue;
            }

            // 将网卡地址加入到监听列表中
            for (auto &x : result) {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if (ipaddr) {
                    ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                }
                address.push_back(ipaddr);
            }
        }

        // 启动 httpserver 服务器
        sylar::http::HttpServer::ptr server(
            new sylar::http::HttpServer(i.keepalive));

        // 将服务器和地址进行绑定
        std::vector<Address::ptr> fails;
        if (!server->bind(address, fails)) {
            for (auto &x : fails) {
                SYLAR_LOG_ERROR(g_logger) << "bind address faill:" << *x;
            }
            _exit(0);
        }

        // 设置服务器的名字, 并启动服务
        if (!i.name.empty()) {
            server->setName(i.name);
        }
        server->start();
        m_httpservers.push_back(server);
    }

    // // 持续运行服务
    // while (true) {
    //     SYLAR_LOG_INFO(g_logger) << "hello world";
    //     usleep(1000 * 10);
    // }

    return 0;
}

}; // namespace sylar

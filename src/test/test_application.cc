#include "sylar/application.hh"
#include <iostream>

int main(int argc, char *argv[])
{
    sylar::Application app;
    if (app.init(argc, argv)) {
        std::cout << "init success" << std::endl;
        return app.run();
    }

    return 0;
}

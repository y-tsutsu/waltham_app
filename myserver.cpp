#include <iostream>
#include <memory>
#include <ev++.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class MyServer
{
private:
    int socket_;
    ev::default_loop &loop_;
    std::unique_ptr<ev::io> watcher_;
    std::unique_ptr<ev::prepare> prepare_;

    static const uint16_t TCP_PORT = 34400;

public:
    MyServer(ev::default_loop &loop)
        : loop_(loop), watcher_(std::make_unique<ev::io>(loop)), prepare_(std::make_unique<ev::prepare>(loop))
    {
        this->socket_ = this->StartListen();
        this->watcher_->set<MyServer, &MyServer::AcceptClient>(this);
        this->watcher_->set(this->GetSocket(), ev::READ);
        this->watcher_->start();
    }

private:
    int StartListen()
    {
        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(MyServer::TCP_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        int reuse = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            std::cerr << "***** Failed to bind to port *****" << TCP_PORT << std::endl;
            close(fd);
            return -1;
        }

        if (::listen(fd, 1024) < 0)
        {
            std::cerr << "***** Failed to listen to port *****" << TCP_PORT << std::endl;
            close(fd);
            return -1;
        }

        return fd;
    }

    void AcceptClient()
    {

    }

    int GetSocket() { return 0; }
};

int main(int argc, char *argv[])
{
    try
    {
        std::cout << "***** My Server. *****" << std::endl;

        ev::default_loop loop;
        auto server = MyServer(loop);
        loop.run(0);

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

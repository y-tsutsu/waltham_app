#include <waltham-server.h>
#include <waltham-connection.h>
#include <ev++.h>

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <unistd.h>
#include <string.h>

#include <memory>
#include <set>

static void display_handle_client_version(struct wth_display *wth_display,
                                          uint32_t client_version)
{
    wth_object_post_error((struct wth_object *)wth_display, 0,
                          "unimplemented: %s", __func__);
}

static void display_handle_sync(struct wth_display *wth_display,
                                struct wthp_callback *callback)
{
    auto *c = static_cast<class client *>(
        wth_object_get_user_data((struct wth_object *)wth_display));

    fprintf(stderr, "Client %p requested wth_display.sync\n", c);
    wthp_callback_send_done(callback, 0);
    wthp_callback_free(callback);
}

static void display_handle_get_registry(struct wth_display *wth_display,
                                        struct wthp_registry *registry)
{
    auto *c = static_cast<class client *>(
        wth_object_get_user_data((struct wth_object *)wth_display));

    fprintf(stderr, "display_handle_get_registry\n");

#if 0 // TODO: 
    struct registry* reg;
    reg = zalloc(sizeof *reg);
    if (!reg) {
        client_post_out_of_memory(c);
        return;
    }
    reg->obj = registry;
    reg->client = c;
    wl_list_insert(&c->registry_list, &reg->link);
    wthp_registry_set_interface(registry, &registry_implementation, reg);
    /* XXX: advertise our globals */
    wthp_registry_send_global(registry, 1, "wthp_compositor", 4);
#endif
}

static const struct wth_display_interface display_implementation = {
    display_handle_client_version, display_handle_sync,
    display_handle_get_registry};

class client
{
    std::unique_ptr<ev::io> watcher_;
    wth_connection *const conn_;

  public:
    client(ev::default_loop &loop, wth_connection *&&conn)
        : watcher_(std::make_unique<ev::io>(loop)), conn_(conn)
    {
        int fd = wth_connection_get_fd(conn_);
        watcher_->set<client, &client::receive>(this);
        watcher_->set(fd, ev::READ);

        watcher_->start();

        /* XXX: this should be inside Waltham */
        auto disp = wth_connection_get_display(conn_);
        wth_display_set_interface(disp, &display_implementation, this);
    }

    void receive()
    {
        std::cerr << __func__ << std::endl;
        auto fd = wth_connection_get_fd(conn_);
        fprintf(stderr, "@@@@@@@@@ fd = %d\n", fd);

        if (wth_connection_read(conn_) < 0)
        {
            fprintf(stderr, "Client %p read error.\n", this);
            // client_destroy(c);

            return;
        }

        if (wth_connection_dispatch(conn_) < 0 && errno != EPROTO)
        {
            fprintf(stderr, "Client %p dispatch error.\n", this);
            // client_destroy(c);

            return;
        }
    }

    void flush()
    {
        // TODO: error handle
        wth_connection_flush(conn_);
    }
};

class server
{
    int socket_;
    ev::default_loop &loop_;
    std::unique_ptr<ev::io> watcher_;
    std::unique_ptr<ev::prepare> prepare_;
    std::set<client *> clients_;

  public:
    server(ev::default_loop &loop)
        : loop_(loop), watcher_(std::make_unique<ev::io>(loop)), prepare_(std::make_unique<ev::prepare>(loop))
    {
        socket_ = start_listen();

        watcher_->set<server, &server::accept_client>(this);
        watcher_->set(this->get_socket(), ev::READ);
        watcher_->start();

        prepare_->set<server, &server::before>(this);
        prepare_->start();
    }

    void before()
    {
        fprintf(stderr, "@@@@@@@@@@@@ before\n");
        for (auto &&c : clients_)
        {
            c->flush();
        }
    }

    int start_listen()
    {
        // FIXME:
        static const uint16_t tcp_port = 34400;

        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(tcp_port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        int reuse = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

        if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0)
        {
            std::cerr << "Failed to bind to port" << tcp_port << std::endl;
            close(fd);
            return -1;
        }

        if (listen(fd, 1024) < 0)
        {
            std::cerr << "Failed to listen to port" << tcp_port << std::endl;
            close(fd);
            return -1;
        }

        return fd;
    }

    void accept_client()
    {
        // struct client *client;
        struct sockaddr_in addr;
        socklen_t len;

        len = sizeof addr;
        struct wth_connection *conn =
            wth_accept(socket_, (struct sockaddr *)&addr, &len);
        if (!conn)
        {
            fprintf(stderr, "Failed to accept a connection.\n");

            return;
        }

        // FIXME: memory leak.
        clients_.insert(new client(loop_, std::move(conn)));

        // if (!client) {
        //     fprintf(stderr, "Failed client_create().\n");

        //     return;
        // }
    }

    int get_socket() { return socket_; }
};

int main(int argc, char *argv[])
{
    try
    {
        ev::default_loop loop;

        server srv(loop);

        loop.run(0);

        return 0;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

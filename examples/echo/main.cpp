#include <coco/network.hpp>

#include <cstring>

using namespace coco;

class echo final : public tcp_server {
public:
    using tcp_server::tcp_server;
    auto on_accept(tcp_connection connection) noexcept -> task<void> final;
};

auto main(int argc, char **argv) -> int {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -10;
    }

    std::error_code error;

    auto address = ip_address::ipv4_loopback();
    auto port    = static_cast<uint16_t>(atoi(argv[1]));

    echo server;
    error = server.listen(address, port);
    if (error.value() != 0) {
        fprintf(stderr, "Failed to listen to 127.0.0.1:%hu - %s\n", port,
                error.message().c_str());
        return EXIT_FAILURE;
    }

    io_context io_ctx{1};
    server.run(io_ctx);

    return 0;
}

auto echo::on_accept(tcp_connection connection) noexcept -> task<void> {
    char            buffer[4096];
    size_t          bytes;
    std::error_code error;

    for (;;) {
        memset(buffer, 0, sizeof(buffer));
        error = co_await connection.receive(buffer, sizeof(buffer), bytes);
        if (error.value() != 0) [[unlikely]] {
            printf(
                "[Error] Failed to receive message: %d. Closing Connection.\n",
                error.value());
            break;
        }

        if (bytes == 0) [[unlikely]] {
            printf("[Info] Connection closed with %s:%hu\n",
                   connection.address().to_string().data(), connection.port());
            break;
        }

        fprintf(stderr, "[Info] received %zu bytes of data.\n", bytes);

        error = co_await connection.send(buffer, bytes, bytes);
        if (error.value() != 0) [[unlikely]] {
            printf("[Error] Failed to send message: %s. Closing Connection.\n",
                   error.message().data());
            break;
        }

        fprintf(stderr, "[Info] sent %zu bytes of data.\n", bytes);
    }

    fprintf(stderr, "[Info] Connection with %s:%hu closing.\n",
            connection.address().to_string().c_str(), connection.port());
    co_return;
}

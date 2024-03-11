#include <coco/network.hpp>

#include <cstring>

using namespace coco;

auto echo(tcp_connection connection) noexcept -> task<void> {
    char            buffer[4096];
    size_t          bytes;
    std::error_code error;

    for (;;) {
        memset(buffer, 0, sizeof(buffer));
        error = co_await connection.receive(buffer, sizeof(buffer), bytes);
        if (error.value() != 0) [[unlikely]] {
            fprintf(
                stderr,
                "[Error] Failed to receive message: %d. Closing Connection.\n",
                error.value());
            break;
        }

        if (bytes == 0) [[unlikely]] {
            printf("[Info] Connection closed with %s:%hu\n",
                   connection.address().to_string().data(), connection.port());
            break;
        }

        printf("[Info] received %zu bytes of data.\n", bytes);

        error = co_await connection.send(buffer, bytes, bytes);
        if (error.value() != 0) [[unlikely]] {
            printf("[Error] Failed to send message: %s. Closing Connection.\n",
                   error.message().data());
            break;
        }

        printf("[Info] sent %zu bytes of data.\n", bytes);
    }

    printf("[Info] Connection with %s:%hu closing.\n",
           connection.address().to_string().c_str(), connection.port());
    co_return;
}

auto acceptor(io_context &io_ctx, tcp_server server) noexcept -> task<void> {
    std::error_code error;
    tcp_connection  connection;
    for (;;) {
        error = co_await server.accept(connection);
        if (error.value() != 0) [[unlikely]] {
            fprintf(stderr,
                    "[Error] TCP server failed to accept new connection.\n");
            io_ctx.stop();
            co_return;
        }

        io_ctx.execute(echo(std::move(connection)));
    }
}

auto main(int argc, char **argv) -> int {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -10;
    }

    std::error_code error;

    auto address = ip_address::ipv4_loopback();
    auto port    = static_cast<uint16_t>(atoi(argv[1]));

    tcp_server server;
    error = server.listen(address, port);
    if (error.value() != 0) {
        fprintf(stderr, "Failed to listen to 127.0.0.1:%hu - %s\n", port,
                error.message().c_str());
        return EXIT_FAILURE;
    }

    io_context io_ctx{1};
    io_ctx.execute(acceptor(io_ctx, std::move(server)));
    io_ctx.run();

    return 0;
}

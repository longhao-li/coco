#include <coco/file.hpp>

using namespace coco;

auto copy(io_context &io_ctx, const char *from, const char *to) noexcept
    -> task<void> {
    std::error_code error;
    size_t          size;
    uint32_t        bytes;

    binary_file src;
    binary_file dest;

    char buffer[4194304];

    error = src.open(from, binary_file::flag::read);
    if (error.value() != 0) {
        fprintf(stderr, "[Error] Failed to open source file %s: %s\n", from,
                error.message().c_str());
        io_ctx.stop();
        co_return;
    }

    error = dest.open(to, binary_file::flag::write | binary_file::flag::trunc);
    if (error.value() != 0) {
        fprintf(stderr, "[Error] Failed to open file %s: %s\n", from,
                error.message().c_str());
        io_ctx.stop();
        co_return;
    }

    size = src.size();
    while (size >= sizeof(buffer)) {
        error = co_await src.read(buffer, sizeof(buffer), bytes);
        if (error.value() != 0) {
            fprintf(stderr, "[Error] Failed to read data from file %s: %s\n",
                    from, error.message().c_str());
            break;
        }

        error = co_await dest.write(buffer, bytes);
        if (error.value() != 0) {
            fprintf(stderr, "[Error] Failed to write data to file %s: %s\n", to,
                    error.message().c_str());
            break;
        }

        size -= bytes;
        printf("rest size: %zu bytes\n", size);
    }

    if (size != 0) {
        error = co_await src.read(buffer, size, bytes);
        if (error.value() != 0) {
            fprintf(stderr, "[Error] Failed to read data from file %s: %s\n",
                    from, error.message().c_str());
            io_ctx.stop();
            co_return;
        }

        error = co_await dest.write(buffer, bytes);
        if (error.value() != 0) {
            fprintf(stderr, "[Error] Failed to write data to file %s: %s\n", to,
                    error.message().c_str());
            io_ctx.stop();
            co_return;
        }

        size -= sizeof(buffer);
    }

    io_ctx.stop();
}

auto main(int argc, char **argv) -> int {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <src> <dest>\n", argv[0]);
        return -10;
    }

    io_context io_ctx{1};
    io_ctx.execute(copy(io_ctx, argv[1], argv[2]));
    io_ctx.run();

    return 0;
}

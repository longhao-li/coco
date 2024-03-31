#include <coco/sync.hpp>

using namespace coco;

auto loop(int id) noexcept -> task<void> {
    for (;;) {
        printf("[Info] Yield loop %d.\n", id);
        co_await yield();
    }
}

auto main() -> int {
    io_context io_ctx{1};

    io_ctx.execute(loop(0));
    io_ctx.execute(loop(1));
    io_ctx.execute(loop(2));
    io_ctx.execute(loop(3));
    io_ctx.execute(loop(4));
    io_ctx.execute(loop(5));
    io_ctx.execute(loop(6));
    io_ctx.execute(loop(7));
    io_ctx.execute(loop(8));
    io_ctx.execute(loop(9));
    io_ctx.run();

    return 0;
}

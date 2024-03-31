#include <coco/io.hpp>
#include <coco/sync.hpp>

#include <iostream>

using namespace coco;
using namespace std::chrono_literals;

auto loop(io_context &ctx, int id, mutex &mtx, size_t &count) noexcept
    -> task<void> {
    timer  t;
    size_t current = 0;
    for (;;) {
        {
            std::lock_guard<mutex> guard{mtx};
            current = count++;
        }

        std::cout << "[" << std::this_thread::get_id() << "] [" << id
                  << "] Count: " << current << "\n";

        if (current >= 200)
            ctx.stop();

        co_await t.wait(50ms);
    }
}

auto main() -> int {
    io_context io_ctx{4};

    size_t count = 0;
    mutex  mtx;

    io_ctx.execute(loop(io_ctx, 0, mtx, count));
    io_ctx.execute(loop(io_ctx, 1, mtx, count));
    io_ctx.execute(loop(io_ctx, 2, mtx, count));
    io_ctx.execute(loop(io_ctx, 3, mtx, count));
    io_ctx.execute(loop(io_ctx, 4, mtx, count));
    io_ctx.execute(loop(io_ctx, 5, mtx, count));
    io_ctx.execute(loop(io_ctx, 6, mtx, count));
    io_ctx.execute(loop(io_ctx, 7, mtx, count));
    io_ctx.execute(loop(io_ctx, 8, mtx, count));
    io_ctx.execute(loop(io_ctx, 9, mtx, count));
    io_ctx.execute(loop(io_ctx, 10, mtx, count));
    io_ctx.execute(loop(io_ctx, 11, mtx, count));
    io_ctx.execute(loop(io_ctx, 12, mtx, count));
    io_ctx.execute(loop(io_ctx, 13, mtx, count));
    io_ctx.execute(loop(io_ctx, 14, mtx, count));
    io_ctx.execute(loop(io_ctx, 15, mtx, count));

    io_ctx.run();
    return 0;
}

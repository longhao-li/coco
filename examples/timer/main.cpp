#include <coco/io.hpp>

#include <iostream>

using namespace coco;
using namespace std::chrono_literals;

template <class Rep, class Period>
auto loop(std::chrono::duration<Rep, Period> interval) noexcept -> task<void> {
    timer t;
    for (;;) {
        std::error_code error = co_await t.wait(interval);
        if (error.value() != 0) {
            std::cerr << "[Error] Failed to suspend timer: " << error.message()
                      << ".\n";
            break;
        }

        std::cout << "[Info] Coroutine suspended for " << interval << ".\n";
    }
}

auto main() -> int {
    io_context io_ctx{1};
    io_ctx.execute(loop(500ms));
    io_ctx.execute(loop(10s));
    io_ctx.run();
    return 0;
}

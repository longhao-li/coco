#include "coco/timer.hpp"

#include <sys/timerfd.h>

using namespace coco;

coco::timer::timer() noexcept
    : m_timer{timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)} {
    assert(m_timer >= 0);
}

coco::timer::~timer() {
    if (m_timer != -1)
        close(m_timer);
}

auto coco::timer::operator=(timer &&other) noexcept -> timer & {
    if (this == &other) [[unlikely]]
        return *this;

    if (m_timer != -1)
        close(m_timer);

    m_timer       = other.m_timer;
    other.m_timer = -1;

    return *this;
}

namespace {

struct wait_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int      m_timer;
    uint64_t m_buffer;

    auto await_ready() noexcept -> bool {
        return false;
    }

    auto await_suspend(coroutine_handle coroutine) noexcept -> void {
        m_userdata.coroutine = coroutine.address();
        m_userdata.cqe_res   = 0;
        m_userdata.cqe_flags = 0;

        auto *worker = coroutine.promise().worker();
        assert(worker != nullptr);

        io_uring     *ring = worker->io_ring();
        io_uring_sqe *sqe  = io_uring_get_sqe(ring);
        while (sqe == nullptr) [[unlikely]] {
            io_uring_submit(ring);
            sqe = io_uring_get_sqe(ring);
        }

        io_uring_prep_read(sqe, m_timer, &m_buffer, sizeof(m_buffer), 0);
        sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

        int result = io_uring_submit(ring);
        assert(result >= 1);
        (void)result;
    }

    auto await_resume() noexcept -> int {
        return m_userdata.cqe_res;
    }
};

} // namespace

auto coco::timer::wait(int64_t nanoseconds) noexcept -> task<std::error_code> {
    int64_t seconds  = nanoseconds / 1000000000;
    nanoseconds     %= 1000000000;

    itimerspec timeout{
        .it_interval = {},
        .it_value    = {.tv_sec = seconds, .tv_nsec = nanoseconds},
    };

    int result = timerfd_settime(m_timer, 0, &timeout, nullptr);
    if (result < 0) [[unlikely]]
        co_return std::error_code{result, std::system_category()};

    wait_awaitable awaitable{
        .m_userdata = {},
        .m_timer    = m_timer,
        .m_buffer   = 0,
    };
    result = co_await awaitable;

    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

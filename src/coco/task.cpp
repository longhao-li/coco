#include "coco/task.hpp"

#include <sys/eventfd.h>

using namespace coco;
using namespace coco::detail;

coco::detail::io_context_worker::io_context_worker() noexcept
    : m_should_exit{false}, m_is_running{false}, m_ring{}, m_wake_up{-1},
      m_wake_up_buffer{}, m_mutex{}, m_tasks{} {
    int result = io_uring_queue_init(1024, &m_ring, IORING_SETUP_SQPOLL);
    assert(result == 0);
    (void)result;

    m_wake_up = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    assert(m_wake_up >= 0);

    m_tasks.reserve(16);
}

coco::detail::io_context_worker::~io_context_worker() {
    assert(!m_is_running.load(std::memory_order_relaxed));

    close(m_wake_up);
    io_uring_queue_exit(&m_ring);

    for (auto &coro : m_tasks)
        coro.destroy();
}

auto coco::detail::io_context_worker::wake_up() noexcept -> void {
    io_uring_sqe *sqe = io_uring_get_sqe(&m_ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(&m_ring);
        sqe = io_uring_get_sqe(&m_ring);
    }

    sqe->user_data   = 0;
    uint64_t value   = 1;
    ssize_t  written = write(m_wake_up, &value, sizeof(value));
    assert(written == sizeof(value));
    (void)written;

    io_uring_prep_read(sqe, m_wake_up, &m_wake_up_buffer,
                       sizeof(m_wake_up_buffer), 0);
    int result = io_uring_submit(&m_ring);
    assert(result >= 1);
    (void)result;
}

auto coco::detail::io_context_worker::run() noexcept -> void {
    if (m_is_running.exchange(true, std::memory_order_acq_rel)) [[unlikely]]
        return;

    __kernel_timespec timeout{
        .tv_sec  = 1,
        .tv_nsec = 0,
    };

    io_uring_cqe *cqe;
    task_list     tasks;
    tasks.reserve(16);

    m_should_exit.store(false, std::memory_order_release);
    while (!m_should_exit.load(std::memory_order_acquire)) {
        timeout = __kernel_timespec{
            .tv_sec  = 1,
            .tv_nsec = 0,
        };

        int result = io_uring_wait_cqe_timeout(&m_ring, &cqe, &timeout);
        while (result == 0) {
            auto *data = static_cast<user_data *>(io_uring_cqe_get_data(cqe));
            if (data == nullptr) {
                io_uring_cq_advance(&m_ring, 1);
                result = io_uring_peek_cqe(&m_ring, &cqe);
                continue;
            }

            data->cqe_res   = cqe->res;
            data->cqe_flags = cqe->flags;

            auto coroutine = std::coroutine_handle<promise_base>::from_address(
                data->coroutine);
            auto stack_bottom = coroutine.promise().stack_bottom();

            assert(!coroutine.done());
            coroutine.resume();
            if (stack_bottom.done())
                stack_bottom.destroy();

            io_uring_cq_advance(&m_ring, 1);
            result = io_uring_peek_cqe(&m_ring, &cqe);
        }

        { // Handle tasks.
            std::lock_guard<std::mutex> lock{m_mutex};
            tasks.swap(m_tasks);
        }

        for (auto &coroutine : tasks) {
            coroutine.resume();
            if (coroutine.done())
                coroutine.destroy();
        }

        tasks.clear();
    }

    m_is_running.store(false, std::memory_order_release);
}

auto coco::detail::io_context_worker::stop() noexcept -> void {
    if (!m_is_running.load(std::memory_order_acquire))
        return;

    m_should_exit.store(true, std::memory_order_release);
    this->wake_up();
}

coco::io_context::io_context() noexcept
    : io_context{std::thread::hardware_concurrency()} {}

coco::io_context::io_context(uint32_t num_workers) noexcept
    : m_workers{}, m_threads{}, m_num_workers{}, m_next_worker{0} {
    num_workers   = (num_workers == 0) ? 1 : num_workers;
    m_num_workers = num_workers;

    m_workers = std::make_unique<io_context_worker[]>(num_workers);
}

coco::io_context::~io_context() {
    this->stop();
}

auto coco::io_context::run() noexcept -> void {
    m_threads = std::make_unique<std::jthread[]>(m_num_workers - 1);
    for (uint32_t i = 0; i < m_num_workers - 1; ++i)
        m_threads[i] =
            std::jthread([worker = &(m_workers[i])] { worker->run(); });

    m_workers[m_num_workers - 1].run();
}

auto coco::io_context::stop() noexcept -> void {
    for (uint32_t i = 0; i < m_num_workers; ++i)
        m_workers[i].stop();
}

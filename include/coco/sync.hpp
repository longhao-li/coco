#pragma once

#include "task.hpp"

#include <queue>
#include <utility>

namespace coco {

/// \class mutex
/// \brief
///   Mutex for tasks.
class mutex;

} // namespace coco

namespace coco {

/// \class yield_awaitable
/// \brief
///   For internal usage. Awaitable object to yield current coroutine.
class [[nodiscard]] yield_awaitable {
public:
    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Append this coroutine to the waiting task list and suspend it
    ///   immediately.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> void {
        detail::io_context_worker *worker = coro.promise().worker();
        worker->execute(coro);
    }

    /// \brief
    ///   Called when current coroutine is resumed. Nothing to do here.
    auto await_resume() noexcept -> void {}
};

/// \class mutex_lock_awaitable
/// \brief
///   Awaitable class for mutex lock operation.
class [[nodiscard]] mutex_lock_awaitable {
public:
    /// \brief
    ///   Initialize a new mutex lock awaitable object.
    mutex_lock_awaitable(mutex *mtx) noexcept : m_mutex{mtx} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Try to acquire current mutex and suspend current coroutine if
    ///   necessary.
    /// \tparam Promise
    ///   Promise type of current coroutine.
    /// \param coro
    ///   Coroutine handle of current coroutine.
    /// \retval true
    ///   This coroutine should be suspended.
    /// \retval false
    ///   This coroutine should not be suspended.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool;

    /// \brief
    ///   Called when current coroutine is resumed. Nothing to do here.
    auto await_resume() noexcept -> void {}

private:
    mutex *m_mutex;
};

} // namespace coco

namespace coco {

/// \brief
///   Yield current coroutine temporarily. This coroutine will be resumed when
///   other coroutines are handled or suspended.
/// \remarks
///   This method is designed for convenience so that user can simply use
///   `co_await coco::yield()` to yield current coroutine. This is exactly the
///   same as `co_await coco::yield_awaitable{}`.
/// \return
///   A new yield_awaitable object.
[[nodiscard]] constexpr auto yield() noexcept -> yield_awaitable {
    return {};
}

/// \class mutex
/// \brief
///   Mutex designed for coroutine.
class mutex {
public:
    /// \brief
    ///   Create and initialize this mutex.
    mutex() noexcept : m_mutex{}, m_is_locked{false}, m_tasks{} {}

    /// \brief
    ///   Mutex is not copyable.
    mutex(const mutex &other) = delete;

    /// \brief
    ///   Mutex is not moveable.
    mutex(mutex &&other) = delete;

    /// \brief
    ///   Destroy this mutex.
    /// \note
    ///   It is undefined behavior if this mutex is locked when destroying.
    ~mutex() = default;

    /// \brief
    ///   Mutex is not copyable.
    auto operator=(const mutex &other) = delete;

    /// \brief
    ///   Mutex is not moveable.
    auto operator=(mutex &&other) = delete;

    /// \brief
    ///   Acquire this mutex. Current coroutine will be suspended if this mutex
    ///   is not available currently and will be resumed once available.
    /// \return
    ///   An awaitable object to acquire this mutex.
    auto lock() noexcept -> mutex_lock_awaitable {
        return {this};
    }

    /// \brief
    ///   Try to acquire this mutex. This method always returns immediately.
    /// \retval true
    ///   This mutex is acquired successfully.
    /// \retval false
    ///   Failed to acquire this mutex.
    auto try_lock() noexcept -> bool {
        std::lock_guard<std::mutex> guard{m_mutex};
        if (std::exchange(m_is_locked, true) == false)
            return true;
        return false;
    }

    /// \brief
    ///   Release this mutex.
    /// \note
    ///   It is undefined behavior to release an unlocked mutex.
    auto unlock() noexcept -> void {
        // Mutex is used here to avoid possible concurrency conflict which maybe
        // a slow implementation.
        std::lock_guard<std::mutex> guard{m_mutex};

        // Resume pending task if there is any.
        if (!m_tasks.empty()) {
            auto [task, worker] = m_tasks.front();
            m_tasks.pop();
            worker->execute(task);
        } else {
            m_is_locked = false;
        }
    }

    friend class mutex_lock_awaitable;

private:
    struct pending_task {
        std::coroutine_handle<>    coroutine;
        detail::io_context_worker *worker;
    };

    std::mutex               m_mutex;
    bool                     m_is_locked;
    std::queue<pending_task> m_tasks;
};

} // namespace coco

namespace coco {

template <class Promise>
auto coco::mutex_lock_awaitable::await_suspend(
    std::coroutine_handle<Promise> coro) noexcept -> bool {
    // Mutex is used here to avoid possible concurrency conflict which maybe a
    // slow implementation.
    std::lock_guard<std::mutex> guard{m_mutex->m_mutex};

    // Check if this mutex is currently locked.
    if (std::exchange(m_mutex->m_is_locked, true) == false)
        return false;

    // Failed to acquire mutex, suspend this coroutine.
    m_mutex->m_tasks.push({coro, coro.promise().worker()});
    return true;
}

} // namespace coco

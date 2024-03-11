#pragma once

#include <liburing.h>

#include <atomic>
#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <thread>
#include <vector>

namespace coco {

/// \class task
/// \tparam T
///   Return type of the coroutine.
/// \brief
///   Task type for coroutines.
template <class T> class task;

/// \class promise
/// \tparam T
///   Return type of the coroutine.
/// \brief
///   Promise type for tasks.
template <class T> class promise;

namespace detail {

/// \class io_context_worker
/// \brief
///   For internal usage. Worker class for io_context.
class io_context_worker;

} // namespace detail
} // namespace coco

namespace coco::detail {

/// \class task_awaitable
/// \tparam T
///   Return type of the coroutine.
/// \brief
///   Awaitable object for tasks.
template <class T> class task_awaitable {
public:
    using value_type       = T;
    using promise_type     = typename coco::task<value_type>::promise_type;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    /// \brief
    ///   For internal usage. Create a new task awaitable for the given
    ///   coroutine. Used by task::operator co_await.
    /// \param coroutine
    ///   Current coroutine handle.
    task_awaitable(coroutine_handle coroutine) : m_coroutine(coroutine) {}

    /// \brief
    ///   For internal usage. Checks if current coroutine should be suspended.
    ///   Empty coroutine or completed coroutine should not be suspended.
    /// \retval true
    ///   Current coroutine should be suspended.
    /// \retval false
    ///   Current coroutine should not be suspended.
    [[nodiscard]] auto await_ready() const noexcept -> bool {
        return (m_coroutine == nullptr) || m_coroutine.done();
    }

    /// \brief
    ///   C++20 coroutine awaitable type method. Suspend the caller coroutine
    ///   and start this one.
    /// \tparam Promise
    ///   Promise type of the caller coroutine.
    /// \param caller
    ///   Caller coroutine handle.
    /// \return
    ///   Coroutine handle of this task.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> caller) noexcept
        -> coroutine_handle;

    /// \brief
    ///   C++20 coroutine awaitable type method. Get result of the coroutine.
    /// \return
    ///   Return value of the task.
    auto await_resume() const noexcept -> value_type {
        return std::move(m_coroutine.promise()).result();
    }

private:
    coroutine_handle m_coroutine;
};

/// \class promise_awaitable
/// \brief
///   Awaitable object for promise final suspend.
class promise_awaitable {
public:
    /// \brief
    ///   For internal usage. Promise final suspend should always be performed.
    /// \return
    ///   This method always returns false.
    [[nodiscard]] auto await_ready() const noexcept -> bool {
        return false;
    }

    /// \brief
    ///   C++20 coroutine awaitable type method. Maintain the coroutine promise
    ///   status and do suspention or resumption.
    /// \tparam Promise
    ///   Promise type of the caller coroutine.
    /// \param coroutine
    ///   Caller coroutine handle.
    /// \return
    ///   A coroutine handle to be resumed. A noop-coroutine handle is returned
    ///   if the caller is done.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coroutine) noexcept
        -> std::coroutine_handle<>;

    /// \brief
    ///   C++20 coroutine awaitable type method. Nothing to do here.
    auto await_resume() const noexcept -> void {}
};

/// \class promise_base
/// \brief
///   Base class for promise types.
class promise_base {
public:
    /// \brief
    ///   Create an empty promise base.
    promise_base() noexcept
        : m_coroutine{}, m_caller_or_top{this}, m_stack_bottom{this},
          m_worker{nullptr} {}

    /// \brief
    ///   Promise is not allowed to be copied.
    promise_base(const promise_base &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    promise_base(promise_base &&other) = delete;

    /// \brief
    ///   Destroy this promise base.
    ~promise_base() = default;

    /// \brief
    ///   Promise is not allowed to be copied.
    auto operator=(const promise_base &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    auto operator=(promise_base &&other) = delete;

    /// \brief
    ///   C++20 coroutine promise type method. Initial suspend of the coroutine.
    ///   Tasks are always suspended once created.
    /// \return
    ///   Always returns std::suspend_always.
    auto initial_suspend() noexcept -> std::suspend_always {
        return {};
    }

    /// \brief
    ///   C++20 coroutine promise type method. Final suspend of the coroutine.
    ///   Maintain the coroutine status and do suspention or resumption.
    /// \return
    ///   A customized promise awaitable object.
    [[nodiscard]] auto final_suspend() const noexcept
        -> detail::promise_awaitable {
        return {};
    }

    /// \brief
    ///   Get current stack top of this coroutine call chain.
    /// \return
    ///   Stack top of this coroutine call chain.
    [[nodiscard]] auto stack_top() const noexcept -> std::coroutine_handle<> {
        return m_stack_bottom->m_caller_or_top->m_coroutine;
    }

    /// \brief
    ///   Get current stack bottom of this coroutine call chain.
    /// \return
    ///   Stack bottom of this coroutine call chain.
    [[nodiscard]] auto stack_bottom() const noexcept
        -> std::coroutine_handle<> {
        return m_stack_bottom->m_coroutine;
    }

    /// \brief
    ///   Set I/O context for current coroutine.
    /// \param[in] io_ctx
    ///   I/O context to be set for current coroutine.
    auto set_worker(io_context_worker *io_ctx) noexcept -> void {
        m_stack_bottom->m_worker.store(io_ctx, std::memory_order_release);
    }

    /// \brief
    ///   Get I/O context for current coroutine.
    /// \return
    ///   I/O context for current coroutine.
    [[nodiscard]] auto worker() const noexcept -> io_context_worker * {
        return m_stack_bottom->m_worker.load(std::memory_order_acquire);
    }

    template <class> friend class coco::task;
    template <class> friend class coco::detail::task_awaitable;
    friend class coco::detail::promise_awaitable;

protected:
    std::coroutine_handle<>          m_coroutine;
    promise_base                    *m_caller_or_top;
    promise_base                    *m_stack_bottom;
    std::atomic<io_context_worker *> m_worker;
};

} // namespace coco::detail

namespace coco {

/// \class task
/// \tparam T
///   Return type of the coroutine.
template <class T> class promise : public detail::promise_base {
public:
    using value_type       = T;
    using reference        = value_type &;
    using rvalue_reference = value_type &&;

    /// \brief
    ///   Create an empty promise.
    promise() noexcept : detail::promise_base{}, m_kind{value_kind::null} {}

    /// \brief
    ///   Promise is not allowed to be copied.
    promise(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    promise(promise &&other) = delete;

    /// \brief
    ///   Destroy this promise.
    ~promise();

    /// \brief
    ///   Promise is not allowed to be copied.
    auto operator=(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    auto operator=(promise &&other) = delete;

    /// \brief
    ///   C++20 coroutine promise type method. Get the coroutine object for this
    ///   promise.
    /// \return
    ///   A task to the coroutine object.
    auto get_return_object() noexcept -> task<value_type>;

    /// \brief
    ///   C++20 coroutine promise type method. Catch and store unhandled
    ///   exception in this promise if exists.
    auto unhandled_exception() noexcept -> void;

    /// \brief
    ///   C++20 coroutine promise type method. Set the return value of the
    ///   coroutine.
    /// \tparam From
    ///   Type of the returned object to be used to construct the actual return
    ///   value.
    /// \param value
    ///   Returnd object of the coroutine.
    template <class From, class = std::enable_if_t<
                              std::is_constructible_v<value_type, From &&>>>
    auto return_value(From &&value) noexcept(
        std::is_nothrow_constructible_v<value_type, From &&>) -> void;

    /// \brief
    ///   C++20 coroutine promise type method. Get return value of the
    ///   coroutine. Exceptions may be thrown if caught any by the coroutine.
    /// \return
    ///   Return reference to return value of the coroutine.
    [[nodiscard]] auto result() & -> reference;

    /// \brief
    ///   C++20 coroutine promise type method. Get return value of the
    ///   coroutine. Exceptions may be thrown if caught any by the coroutine.
    /// \return
    ///   Return reference to return value of the coroutine.
    [[nodiscard]] auto result() && -> rvalue_reference;

private:
    enum class value_kind : intptr_t {
        null,
        exception,
        value,
    };

    value_kind m_kind;
    union data { // NOLINT
        data() noexcept : null{nullptr} {}
        ~data() noexcept {}

        std::nullptr_t     null;
        std::exception_ptr exception;
        alignas(T) char storage[sizeof(T)];
    } m_value;
};

/// \class promise<void>
/// \brief
///   Specialized promise type for tasks returning void.
template <> class promise<void> : public detail::promise_base {
public:
    using value_type = void;

    /// \brief
    ///   Create an empty promise.
    promise() noexcept : detail::promise_base{}, m_exception{} {}

    /// \brief
    ///   Promise is not allowed to be copied.
    promise(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    promise(promise &&other) = delete;

    /// \brief
    ///   Destroy this promise.
    ~promise() = default;

    /// \brief
    ///   Promise is not allowed to be copied.
    auto operator=(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    auto operator=(promise &&other) = delete;

    /// \brief
    ///   C++20 coroutine promise type method. Get the coroutine object for this
    ///   promise.
    /// \return
    ///   A task to the coroutine object.
    auto get_return_object() noexcept -> task<value_type>;

    /// \brief
    ///   C++20 coroutine promise type method. Catch and store unhandled
    ///   exception in this promise if exists.
    auto unhandled_exception() noexcept -> void {
        m_exception = std::current_exception();
    }

    /// \brief
    ///   C++20 coroutine promise type method. Marks that this coroutine has no
    ///   return value.
    auto return_void() noexcept -> void {}

    /// \brief
    ///   C++20 coroutine promise type method. Get return value of the
    ///   coroutine. Exceptions may be thrown if caught any by the coroutine.
    auto result() -> void {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
    }

    template <class> friend class coco::detail::task_awaitable;
    friend class coco::detail::promise_awaitable;

private:
    std::exception_ptr m_exception;
};

/// \class promise
/// \brief
///   Specialized promise type for tasks returning reference.
template <class T> class promise<T &> : public detail::promise_base {
public:
    using value_type = T;
    using reference  = value_type &;

    /// \brief
    ///   Create an empty promise.
    promise() noexcept
        : detail::promise_base{}, m_kind{value_kind::null}, m_value{nullptr} {}

    /// \brief
    ///   Promise is not allowed to be copied.
    promise(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    promise(promise &&other) = delete;

    /// \brief
    ///   Destroy this promise.
    ~promise() = default;

    /// \brief
    ///   Promise is not allowed to be copied.
    auto operator=(const promise &other) = delete;

    /// \brief
    ///   Promise is not allowed to be moved.
    auto operator=(promise &&other) = delete;

    /// \brief
    ///   C++20 coroutine promise type method. Get the coroutine object for this
    ///   promise.
    /// \return
    ///   A coroutine handle to the coroutine object.
    auto get_return_object() noexcept -> task<reference>;

    /// \brief
    ///   C++20 coroutine promise type method. Catch and store unhandled
    ///   exception in this promise if exists.
    auto unhandled_exception() noexcept -> void;

    /// \brief
    ///   C++20 coroutine promise type method. Set the return value of the
    ///   coroutine.
    /// \param value
    ///   Return value of the coroutine.
    auto return_value(reference value) noexcept -> void;

    /// \brief
    ///   C++20 coroutine promise type method. Get return value of the
    ///   coroutine. Exceptions may be thrown if caught any by the coroutine.
    /// \return
    ///   Return reference to return value of the coroutine.
    [[nodiscard]] auto result() -> reference;

private:
    enum class value_kind : intptr_t {
        null,
        exception,
        value,
    };

    value_kind m_kind;
    union data { // NOLINT
        data() noexcept {}
        ~data() noexcept {}

        std::nullptr_t     null;
        std::exception_ptr exception;
        value_type        *value;
    } m_value;
};

/// \class task
/// \tparam T
///   Return type of the coroutine.
/// \class
///   Task type for coroutines.
template <class T> class task {
public:
    using value_type       = T;
    using promise_type     = promise<value_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    /// \brief
    ///   Create an empty task. Empty task is not a valid coroutine.
    task() noexcept : m_coroutine{} {}

    /// \brief
    ///   Create a task from the given coroutine.
    /// \param coroutine
    ///   Coroutine handle of current task coroutine.
    explicit task(coroutine_handle coroutine) noexcept
        : m_coroutine{coroutine} {}

    /// \brief
    ///   Task is not allowed to be copied.
    task(const task &other) = delete;

    /// \brief
    ///   Move constructor for task.
    /// \param other
    ///   The task to be moved. The moved task will be set to empty.
    task(task &&other) noexcept : m_coroutine{other.m_coroutine} {
        other.m_coroutine = nullptr;
    }

    /// \brief
    ///   Destroy this task and the coroutine context.
    ~task() {
        if (m_coroutine != nullptr) [[likely]]
            m_coroutine.destroy();
    }

    /// \brief
    ///   Task is not allowed to be copied.
    auto operator=(const task &other) = delete;

    /// \brief
    ///   Move assignment for task.
    /// \param other
    ///   The task to be moved. The moved task will be set to empty.
    auto operator=(task &&other) noexcept -> task &;

    /// \brief
    ///   Checks if this task is empty.
    /// \retval true
    ///   This task is empty.
    /// \retval false
    ///   This task is not empty.
    [[nodiscard]] auto empty() const noexcept -> bool {
        return (m_coroutine == nullptr);
    }

    /// \brief
    ///   Checks if this task is empty or done.
    /// \retval true
    ///   This task is empty or done.
    /// \retval false
    ///   This task is not empty and not done.
    [[nodiscard]] auto done() const noexcept -> bool {
        return (m_coroutine == nullptr) || m_coroutine.done();
    }

    /// \brief
    ///   Resume this task coroutine if not finished.
    auto resume() -> void;

    /// \brief
    ///   Get return value of the coroutine. Exceptions may be thrown if caught
    ///   any by the coroutine.
    /// \note
    ///   This method does not check if the coroutine is done. It is undefined
    ///   behavior to call this on a non-finished coroutine.
    /// \return
    ///   Return value of thie coroutine.
    auto result() -> value_type {
        return std::move(m_coroutine.promise()).result();
    }

    /// \brief
    ///   Detach the coroutine handle from this task. The task will be set to
    ///   empty.
    /// \return
    ///   Detached coroutine handle of this task coroutine.
    [[nodiscard]] auto detach() noexcept -> coroutine_handle {
        auto coroutine = m_coroutine;
        m_coroutine    = nullptr;
        return coroutine;
    }

    /// \brief
    ///   Get address for this coroutine frame.
    /// \return
    ///   Address of the coroutine frame.
    [[nodiscard]] auto address() const noexcept -> void * {
        return m_coroutine.address();
    }

    /// \brief
    ///   C++20 coroutine awaitable type method. Suspend the caller coroutine
    ///   and start this one.
    auto operator co_await() const noexcept
        -> detail::task_awaitable<value_type> {
        return detail::task_awaitable<value_type>{m_coroutine};
    }

private:
    coroutine_handle m_coroutine;
};

} // namespace coco

namespace coco::detail {

template <class T>
template <class Promise>
auto task_awaitable<T>::await_suspend(
    std::coroutine_handle<Promise> caller) noexcept -> coroutine_handle {
    // Set caller for this coroutine.
    promise_base &base = static_cast<promise_base &>(m_coroutine.promise());
    promise_base &caller_base = static_cast<promise_base &>(caller.promise());
    base.m_caller_or_top      = &caller_base;

    // Maintain stack bottom and top.
    promise_base *stack_bottom = caller_base.m_stack_bottom;
    assert(stack_bottom == stack_bottom->m_stack_bottom);

    base.m_stack_bottom           = stack_bottom;
    stack_bottom->m_caller_or_top = &base;

    return m_coroutine;
}

template <class Promise>
auto promise_awaitable::await_suspend(
    std::coroutine_handle<Promise> coroutine) noexcept
    -> std::coroutine_handle<> {
    promise_base &base  = static_cast<promise_base &>(coroutine.promise());
    promise_base *stack = base.m_stack_bottom;

    assert(stack->m_caller_or_top == &base);
    assert(stack->m_stack_bottom == stack);

    // Stack bottom completed. Nothing to resume.
    if (stack == &base)
        return std::noop_coroutine();

    // Set caller coroutine as the top of the stack.
    promise_base *caller   = base.m_caller_or_top;
    stack->m_caller_or_top = caller;

    // Resume caller.
    return caller->m_coroutine;
}

} // namespace coco::detail

namespace coco {

template <class T> promise<T>::~promise() {
    if (m_kind == value_kind::value) [[likely]]
        reinterpret_cast<T *>(m_value.storage)->~T();
    else if (m_kind == value_kind::exception) [[unlikely]]
        m_value.exception.~exception_ptr();
}

template <class T> auto promise<T>::get_return_object() noexcept -> task<T> {
    auto coroutine    = std::coroutine_handle<promise>::from_promise(*this);
    this->m_coroutine = coroutine;
    return task<T>{coroutine};
}

template <class T> auto promise<T>::unhandled_exception() noexcept -> void {
    assert(m_kind == value_kind::null);
    m_kind = value_kind::exception;
    ::new (static_cast<void *>(&m_value.exception))
        std::exception_ptr{std::current_exception()};
}

template <class T>
template <class From, class>
auto promise<T>::return_value(From &&value) noexcept(
    std::is_nothrow_constructible_v<value_type, From &&>) -> void {
    assert(m_kind == value_kind::null);
    m_kind = value_kind::value;
    ::new (static_cast<void *>(m_value.storage))
        value_type{std::forward<From>(value)};
}

template <class T> auto promise<T>::result() & -> reference {
    if (m_kind == value_kind::exception) [[unlikely]]
        std::rethrow_exception(m_value.exception);

    assert(m_kind == value_kind::value);
    return *reinterpret_cast<value_type *>(m_value.storage);
}

template <class T> auto promise<T>::result() && -> rvalue_reference {
    if (m_kind == value_kind::exception) [[unlikely]]
        std::rethrow_exception(m_value.exception);

    assert(m_kind == value_kind::value);
    return std::move(*reinterpret_cast<value_type *>(m_value.storage));
}

inline auto promise<void>::get_return_object() noexcept -> task<void> {
    auto coroutine    = std::coroutine_handle<promise>::from_promise(*this);
    this->m_coroutine = coroutine;
    return task<void>{coroutine};
}

template <class T>
auto promise<T &>::get_return_object() noexcept -> task<reference> {
    auto coroutine    = std::coroutine_handle<promise>::from_promise(*this);
    this->m_coroutine = coroutine;
    return task<T &>{coroutine};
}

template <class T> auto promise<T &>::unhandled_exception() noexcept -> void {
    assert(m_kind == value_kind::null);
    m_kind = value_kind::exception;
    ::new (static_cast<void *>(&m_value.exception))
        std::exception_ptr{std::current_exception()};
}

template <class T>
auto promise<T &>::return_value(reference value) noexcept -> void {
    assert(m_kind == value_kind::null);
    m_kind        = value_kind::value;
    m_value.value = std::addressof(value);
}

template <class T> auto promise<T &>::result() -> reference {
    if (m_kind == value_kind::exception) [[unlikely]]
        std::rethrow_exception(m_value.exception);

    assert(m_kind == value_kind::value);
    return *m_value.value;
}

template <class T> auto task<T>::operator=(task &&other) noexcept -> task & {
    if (this == &other) [[unlikely]]
        return *this;

    if (m_coroutine != nullptr)
        m_coroutine.destroy();

    m_coroutine       = other.m_coroutine;
    other.m_coroutine = nullptr;

    return *this;
}

template <class T> auto task<T>::resume() -> void {
    if (this->done()) [[unlikely]]
        return;

    auto &base = static_cast<detail::promise_base &>(m_coroutine.promise());
    auto  stack_top = base.m_stack_bottom->m_caller_or_top->m_coroutine;
    stack_top.resume();
}

template <class T>
auto operator==(const task<T> &task, std::nullptr_t) noexcept -> bool {
    return task.empty();
}

template <class T>
auto operator==(std::nullptr_t, const task<T> &task) noexcept -> bool {
    return task.empty();
}

} // namespace coco

namespace coco::detail {

/// \struct user_data
/// \brief
///   Base class for asynchronize IO user data.
struct user_data {
    void    *coroutine;
    int      cqe_res;
    uint32_t cqe_flags;
};

/// \class io_context_worker
/// \brief
///   For internal usage. Worker class for io_context.
class io_context_worker {
public:
    /// \brief
    ///   Create a new worker.
    /// \note
    ///   Program may terminate if failed to prepare IO uring.
    COCO_API io_context_worker() noexcept;

    /// \brief
    ///   io_context_worker is not allowed to be copied.
    io_context_worker(const io_context_worker &other) = delete;

    /// \brief
    ///   io_context_worker is not allowed to be moved.
    io_context_worker(io_context_worker &&other) = delete;

    /// \brief
    ///   Stop this worker and release all resources.
    COCO_API ~io_context_worker();

    /// \brief
    ///   io_context_worker is not allowed to be copied.
    auto operator=(const io_context_worker &other) = delete;

    /// \brief
    ///   io_context_worker is not allowed to be moved.
    auto operator=(io_context_worker &&other) = delete;

    /// \brief
    ///   Wake up this worker immediately.
    COCO_API auto wake_up() noexcept -> void;

    /// \brief
    ///   Start listening for IO events.
    COCO_API auto run() noexcept -> void;

    /// \brief
    ///   Stop listening for IO events. This method will send a stop request and
    ///   return immediately.
    COCO_API auto stop() noexcept -> void;

    /// \brief
    ///   Checks if this worker is running.
    /// \retval true
    ///   This worker is running.
    /// \retval false
    ///   This worker is not running.
    [[nodiscard]] auto is_running() const noexcept -> bool {
        return m_is_running.load(std::memory_order_relaxed);
    }

    /// \brief
    ///   Execute a task in this worker. This method will wake up this worker
    ///   immediately.
    /// \tparam T
    ///   Return type of the task.
    /// \param coro
    ///   The task to be scheduled. This worker will take the ownership of the
    ///   task. The scheduled task will only be resumed once and will be
    ///   destroyed if it is done after the first resume.
    template <class T> auto execute(task<T> coro) noexcept -> void {
        auto  handle = coro.detach();
        auto &base   = static_cast<promise_base &>(handle.promise());
        base.set_worker(this);
        this->schedule(handle);
    }

    /// \brief
    ///   Schedule a task in this worker.
    /// \tparam T
    ///   Return type of the task.
    /// \param coro
    ///   The task to be scheduled. This worker will take the ownership of the
    ///   task. The scheduled task will only be resumed once and will be
    ///   destroyed if it is done after the first resume.
    template <class T> auto schedule(task<T> coro) noexcept -> void {
        auto  handle = coro.detach();
        auto &base   = static_cast<promise_base &>(handle.promise());
        base.set_worker(this);
        this->schedule(handle);
    }

    /// \brief
    ///   For internal usage. Get IO uring of this worker.
    /// \return
    ///   Pointer to the IO uring.
    [[nodiscard]] auto io_ring() noexcept -> io_uring * {
        return &m_ring;
    }

private:
    /// \brief
    ///   Execute a new task in this worker. This method will wake up this
    ///   worker immediately.
    /// \param coro
    ///   The task to be scheduled. The scheduled task will only be resumed once
    ///   and will be destroyed if it is done after the first resume.
    auto execute(std::coroutine_handle<> coro) noexcept -> void {
        { // Append this task to the task list.
            std::lock_guard<std::mutex> lock{m_mutex};
            m_tasks.push_back(coro);
        }

        this->wake_up();
    }

    /// \brief
    ///   Schedule a new task in this worker.
    /// \param coro
    ///   The task to be scheduled. The scheduled task will only be resumed once
    ///   and will be destroyed if it is done after the first resume.
    auto schedule(std::coroutine_handle<> coro) noexcept -> void {
        // Append this task to the task list only.
        std::lock_guard<std::mutex> lock{m_mutex};
        m_tasks.push_back(coro);
    }

private:
    using task_list = std::vector<std::coroutine_handle<>>;

    std::atomic_bool m_should_exit;
    std::atomic_bool m_is_running;
    io_uring         m_ring;
    int              m_wake_up;
    uint64_t         m_wake_up_buffer;

    std::mutex m_mutex;
    task_list  m_tasks;
};

} // namespace coco::detail

namespace coco {

/// \class io_context
/// \brief
///   Working context for asynchronous IO operations.
class io_context {
public:
    /// \brief
    ///   Create a new IO context. Number of workers will be set to the number
    ///   of hardware threads.
    COCO_API io_context() noexcept;

    /// \brief
    ///   Create a new IO context with the specified number of workers.
    /// \param num_workers
    ///   Expected number of workers. This value will be clamped to 1 if it is
    ///   less than 1.
    COCO_API explicit io_context(uint32_t num_workers) noexcept;

    /// \brief
    ///   io_context is not allowed to be copied.
    io_context(const io_context &other) = delete;

    /// \brief
    ///   io_context is not allowed to be moved.
    io_context(io_context &&other) = delete;

    /// \brief
    ///   Stop all workers and release all resources.
    /// \warning
    ///   There may be memory leaks if there are pending I/O tasks scheduled in
    ///   asynchronize I/O handler when stopping. This is due to internal
    ///   implementation of the worker class. This may be fixed in the future.
    COCO_API ~io_context();

    /// \brief
    ///   io_context is not allowed to be copied.
    auto operator=(const io_context &other) = delete;

    /// \brief
    ///   io_context is not allowed to be moved.
    auto operator=(io_context &&other) = delete;

    /// \brief
    ///   Start io_context threads. This method will also block current thread
    ///   and handle IO events and tasks.
    COCO_API auto run() noexcept -> void;

    /// \brief
    ///   Stop all workers. This method will send a stop request and return
    ///   immediately. Workers may not be stopped immediately.
    /// \warning
    ///   There may be memory leaks if there are pending I/O tasks scheduled in
    ///   asynchronize I/O handler when stopping. This is due to internal
    ///   implementation of the worker class. This may be fixed in the future.
    COCO_API auto stop() noexcept -> void;

    /// \brief
    ///   Get number of workers in this io_context.
    /// \return
    ///   Number of workers in this io_context.
    [[nodiscard]] auto size() const noexcept -> size_t {
        return m_num_workers;
    }

    /// \brief
    ///   Execute a new task in a worker. This method will wake up worker
    ///   immediately.
    /// \param coro
    ///   The task to be scheduled. The scheduled task will only be resumed once
    ///   and will be destroyed if it is done after the first resume.
    template <class T> auto execute(task<T> coro) noexcept -> void {
        size_t next = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
                      m_num_workers;
        m_workers[next].execute(std::move(coro));
    }

    /// \brief
    ///   Schedule a new task in a worker.
    /// \param coro
    ///   The task to be scheduled. The scheduled task will only be resumed once
    ///   and will be destroyed if it is done after the first resume.
    template <class T> auto schedule(task<T> coro) noexcept -> void {
        size_t next = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
                      m_num_workers;
        m_workers[next].schedule(std::move(coro));
    }

    /// \brief
    ///   Acquire current worker and update the next worker index.
    /// \return
    ///   Reference to current worker.
    auto acquire_worker() noexcept -> detail::io_context_worker & {
        size_t next = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
                      m_num_workers;
        return m_workers[next];
    }

private:
    using worker_list = std::unique_ptr<detail::io_context_worker[]>;
    using thread_list = std::unique_ptr<std::jthread[]>;

    worker_list          m_workers;
    thread_list          m_threads;
    uint32_t             m_num_workers;
    std::atomic_uint32_t m_next_worker;
};

} // namespace coco

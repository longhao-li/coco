#pragma once

#include "task.hpp"

#include <chrono>

namespace coco {

/// \class timer
/// \brief
///   Timer that is used to suspend tasks for a while.
class timer {
public:
    /// \brief
    ///   Create a new timer object.
    COCO_API timer() noexcept;

    /// \brief
    ///   Timer is not allowed to be copied.
    timer(const timer &other) = delete;

    /// \brief
    ///   Move constructor of timer.
    /// \param other
    ///   The timer to be moved. The moved timer will be invalidated.
    timer(timer &&other) noexcept : m_timer{other.m_timer} {
        other.m_timer = -1;
    }

    /// \brief
    ///   Destroy this timer.
    COCO_API ~timer();

    /// \brief
    ///   Timer is not allowed to be copied.
    auto operator=(const timer &other) = delete;

    /// \brief
    ///   Move assignment of timer.
    /// \param other
    ///   The timer to be moved. The moved timer will be invalidated.
    /// \return
    ///   Reference to this timer.
    COCO_API auto operator=(timer &&other) noexcept -> timer &;

    /// \brief
    ///   Suspend current coroutine for a while.
    /// \note
    ///   There could be at most one wait task for the same timer at the same
    ///   time.
    /// \tparam Rep
    ///   Type representation of the time type. See std::chrono::duration for
    ///   details.
    /// \tparam Period
    ///   Ratio type that is used to measure how to do conversion between
    ///   different duration types. See std::chrono::duration for details.
    /// \param duration
    ///   Time to suspend current coroutine.
    /// \return
    ///   A task that suspends current coroutine for a while. Result of the task
    ///   is a system error code that represents the wait result. Usually it is
    ///   not necessary to check the error code.
    template <class Rep, class Period>
    auto wait(std::chrono::duration<Rep, Period> duration) noexcept
        -> task<std::error_code> {
        std::chrono::nanoseconds nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        return this->wait(nanoseconds.count());
    }

private:
    /// \brief
    ///   For internal usage. Suspend current coroutine for a while.
    /// \param nanoseconds
    ///   Time to suspend this coroutine.
    /// \return
    ///   A task that suspends current coroutine for a while. Result of the task
    ///   is a system error code that represents the wait result. Usually it is
    ///   not necessary to check the error code.
    COCO_API auto wait(int64_t nanoseconds) noexcept -> task<std::error_code>;

private:
    int m_timer;
};

} // namespace coco

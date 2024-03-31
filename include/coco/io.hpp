#pragma once

#include "task.hpp"

#include <chrono>

namespace coco {

/// \class read_awaitable
/// \brief
///   For internal usage. Awaitable object for async read operation.
class [[nodiscard]] read_awaitable {
public:
    /// \brief
    ///   For internal usage. Create a new read awaitable object to prepare for
    ///   async read operation.
    /// \param file
    ///   File descriptor of the file to be read from.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to store the read data.
    /// \param size
    ///   Expected size in byte of data to be read.
    /// \param[out] bytes
    ///   Optional. Used to store how much bytes are read.
    /// \param offset
    ///   Offset in byte of the file to start the read operation. Pass -1 to use
    ///   current file pointer.
    read_awaitable(int file, void *buffer, uint32_t size, uint32_t *bytes,
                   uint64_t offset) noexcept
        : m_userdata{}, m_file{file}, m_size{size}, m_buffer{buffer},
          m_bytes{bytes}, m_offset{offset} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async read operation and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async read operation.
    /// \return
    ///   Error code of the async read operation. The error code is 0 if no
    ///   error occurs.
    auto await_resume() noexcept -> std::error_code {
        if (m_userdata.cqe_res < 0)
            return {-m_userdata.cqe_res, std::system_category()};
        if (m_bytes != nullptr)
            *m_bytes = static_cast<uint32_t>(m_userdata.cqe_res);
        return {};
    }

private:
    /// \brief
    ///   For internal usage. Actually prepare async read operation and suspend
    ///   this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_file;
    uint32_t          m_size;
    void             *m_buffer;
    uint32_t         *m_bytes;
    uint64_t          m_offset;
};

/// \class write_awaitable
/// \brief
///   For internal usage. Awaitable object for async write operation.
class [[nodiscard]] write_awaitable {
public:
    /// \brief
    ///   For internal usage. Create a new write awaitable object to prepare for
    ///   async write operation.
    /// \param file
    ///   File descriptor of the file to be written to.
    /// \param data
    ///   Pointer to start of the buffer to be written.
    /// \param size
    ///   Expected size in byte of data to be written.
    /// \param[out] bytes
    ///   Optional. Used to store how much bytes are written.
    /// \param offset
    ///   Offset in byte of the file to start the write operation. Pass -1 to
    ///   use current file pointer.
    write_awaitable(int file, const void *data, uint32_t size, uint32_t *bytes,
                    uint64_t offset) noexcept
        : m_userdata{}, m_file{file}, m_size{size}, m_data{data},
          m_bytes{bytes}, m_offset{offset} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async write operation and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async write operation.
    /// \return
    ///   Error code of the async write operation. The error code is 0 if no
    ///   error occurs.
    auto await_resume() noexcept -> std::error_code {
        if (m_userdata.cqe_res < 0)
            return {-m_userdata.cqe_res, std::system_category()};
        if (m_bytes != nullptr)
            *m_bytes = static_cast<uint32_t>(m_userdata.cqe_res);
        return {};
    }

private:
    /// \brief
    ///   For internal usage. Actually prepare async write operation and suspend
    ///   this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_file;
    uint32_t          m_size;
    const void       *m_data;
    uint32_t         *m_bytes;
    uint64_t          m_offset;
};

/// \class connect_awaitable
/// \brief
///   For internal usage. Awaitable object for async connect operation.
class [[nodiscard]] connect_awaitable {
public:
    /// \brief
    ///   For internal usage. Try to connect to the specified peer address.
    /// \remarks
    ///   This awaitable will create a new socket and replace the original one
    ///   if succeeded. The original socket will be closed here if succeeded
    ///   which I think is a bad design. But it would be convenient to use if
    ///   creating socket and connect operation are wrapped together.
    /// \param[in,out] target
    ///   The socket to be replaced if succeeded to connect to peer.
    /// \param addr
    ///   Socket address of the peer target.
    /// \param length
    ///   Actual size in byte of the socket address.
    /// \param type
    ///   Type of the new socket.
    /// \param protocol
    ///   Protocol of the new socket.
    connect_awaitable(int *target, const sockaddr_storage &addr,
                      socklen_t length, int type, int protocol) noexcept
        : m_userdata{}, m_socket{-1}, m_type{type}, m_protocol{protocol},
          m_socklen{length}, m_sockaddr{addr}, m_target{target} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async send operation and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async connect
    ///   operation.
    /// \return
    ///   Error code of the async connect operation. The error code is 0 if no
    ///   error occurs.
    COCO_API auto await_resume() noexcept -> std::error_code;

private:
    /// \brief
    ///   For internal usage. Actually prepare async connect operation and
    ///   suspend this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_socket;
    int               m_type;
    int               m_protocol;
    socklen_t         m_socklen;
    sockaddr_storage  m_sockaddr;
    int              *m_target;
};

/// \class send_awaitable
/// \brief
///   For internal usage. Awaitable object for async send operation.
class [[nodiscard]] send_awaitable {
public:
    /// \brief
    ///   For internal usage. Create a new send awaitable object to prepare for
    ///   async send operation.
    /// \param sock
    ///   The socket that this send operation is prepared for.
    /// \param data
    ///   Pointer to start of data to be sent.
    /// \param size
    ///   Expected size in byte of data to be sent.
    /// \param[out] bytes
    ///   Optional. Used to store how much bytes are sent.
    send_awaitable(int sock, const void *data, size_t size,
                   size_t *bytes) noexcept
        : m_userdata{}, m_socket{sock}, m_data{data}, m_size{size},
          m_bytes{bytes} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async send operation and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async send operation.
    /// \return
    ///   Error code of the async send operation. The error code is 0 if no
    ///   error occurs.
    auto await_resume() noexcept -> std::error_code {
        if (m_userdata.cqe_res < 0)
            return {-m_userdata.cqe_res, std::system_category()};
        if (m_bytes != nullptr)
            *m_bytes = static_cast<size_t>(m_userdata.cqe_res);
        return {};
    }

private:
    /// \brief
    ///   For internal usage. Actually prepare async send operation and suspend
    ///   this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_socket;
    const void       *m_data;
    size_t            m_size;
    size_t           *m_bytes;
};

/// \class recv_awaitable
/// \brief
///   For internal usage. Awaitable object for async recv operation.
class [[nodiscard]] recv_awaitable {
public:
    /// \brief
    ///   For internal usage. Create a new recv awaitable object to prepare for
    ///   async recv operation.
    /// \param sock
    ///   The socket that this recv operation is prepared for.
    /// \param buffer
    ///   Pointer to start of the buffer to store the received data.
    /// \param size
    ///   Maximum available size in byte of the receive buffer.
    /// \param[out] bytes
    ///   Optional. Used to store how much bytes are received.
    recv_awaitable(int sock, void *buffer, size_t size, size_t *bytes) noexcept
        : m_userdata{}, m_socket{sock}, m_buffer{buffer}, m_size{size},
          m_bytes{bytes} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async recv operation and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async recv operation.
    /// \return
    ///   Error code of the async recv operation. The error code is 0 if no
    ///   error occurs.
    auto await_resume() noexcept -> std::error_code {
        if (m_userdata.cqe_res < 0)
            return {-m_userdata.cqe_res, std::system_category()};
        if (m_bytes != nullptr)
            *m_bytes = static_cast<size_t>(m_userdata.cqe_res);
        return {};
    }

private:
    /// \brief
    ///   For internal usage. Actually prepare async recv operation and suspend
    ///   this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_socket;
    void             *m_buffer;
    size_t            m_size;
    size_t           *m_bytes;
};

/// \class timeout_awaitable
/// \brief
///   For internal usage. Awaitable object for async timeout operation.
class [[nodiscard]] timeout_awaitable {
public:
    /// \brief
    ///   For internal usage. Create a new timer awaitable object to prepare for
    ///   async timeout operation.
    /// \param timer
    ///   Linux timer file descriptor.
    /// \param nanosecond
    ///   Timeout duration in nanoseconds.
    timeout_awaitable(int timer, int64_t nanosecond) noexcept
        : m_userdata{}, m_timer{timer}, m_nanosecond{nanosecond}, m_buffer{} {}

    /// \brief
    ///   C++20 awaitable internal method. Always execute await_suspend().
    auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   Prepare async timeout and suspend this coroutine.
    /// \tparam Promise
    ///   Type of the promise for the coroutine.
    /// \param coro
    ///   Current coroutine handle.
    /// \return
    ///   A boolean value that specifies whether to suspend current coroutine.
    ///   Current coroutine will be suspended if no error occurs.
    template <class Promise>
    auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> bool {
        m_userdata.coroutine = coro.address();
        return this->suspend(coro.promise().worker());
    }

    /// \brief
    ///   Resume current coroutine and get result of the async timeout event.
    /// \return
    ///   Error code of the async timeout event. The error code is 0 if no error
    ///   occurs.
    auto await_resume() noexcept -> std::error_code {
        if (m_userdata.cqe_res < 0)
            return {-m_userdata.cqe_res, std::system_category()};
        return {};
    }

private:
    /// \brief
    ///   For internal usage. Actually prepare async timeout event and suspend
    ///   this coroutine.
    /// \param[in] worker
    ///   Worker for current thread.
    COCO_API auto suspend(detail::io_context_worker *worker) noexcept -> bool;

private:
    detail::user_data m_userdata;
    int               m_timer;
    int64_t           m_nanosecond;
    int64_t           m_buffer;
};

} // namespace coco

namespace coco {

/// \class ipv4_address
/// \brief
///     Represents an IPv4 address. This is a trivial type and it is safe to do
///     direct memory operations on ipv4_address objects.
class ipv4_address {
public:
    /// \brief
    ///   Create an empty ipv4 address. Empty ipv4 address should not be used
    ///   for any purpose.
    ipv4_address() noexcept = default;

    /// \brief
    ///   Create a new ipv4 address from raw bytes.
    /// \param first
    ///   The first byte of the ipv4 address.
    /// \param second
    ///   The second byte of the ipv4 address.
    /// \param third
    ///   The third byte of the ipv4 address.
    /// \param fourth
    ///   The fourth byte of the ipv4 address.
    constexpr ipv4_address(uint8_t first, uint8_t second, uint8_t third,
                           uint8_t fourth) noexcept
        : m_address{.u8{first, second, third, fourth}} {}

    /// \brief
    ///   ipv4_address is trivially copyable.
    /// \param other
    ///   The ipv4_address to copy from.
    ipv4_address(const ipv4_address &other) noexcept = default;

    /// \brief
    ///   ipv4_address is trivially moveable.
    /// \param other
    ///   The ipv4_address to move from.
    ipv4_address(ipv4_address &&other) noexcept = default;

    /// \brief
    ///   ipv4_address is trivially destructible.
    ~ipv4_address() = default;

    /// \brief
    ///   ipv4_address is trivially copyable.
    /// \param other
    ///   The ipv4_address to copy from.
    /// \return
    ///   A reference to the ipv4_address.
    auto operator=(const ipv4_address &other) noexcept
        -> ipv4_address & = default;

    /// \brief
    ///   ipv4_address is trivially moveable.
    /// \param other
    ///   The ipv4_address to move from.
    /// \return
    ///   A reference to the ipv4_address.
    auto operator=(ipv4_address &&other) noexcept -> ipv4_address & = default;

    /// \brief
    ///   Set a new ipv4 address from string.
    /// \param address
    ///   A string that represents the ipv4 address to be set.
    /// \retval true
    ///   The ipv4 address is set successfully.
    /// \retval false
    ///   The input string is not a valid ipv4 address.
    COCO_API auto set_address(std::string_view address) noexcept -> bool;

    /// \brief
    ///   Get string representation of this ipv4 address.
    /// \return
    ///   A string that represents the ipv4 address.
    [[nodiscard]] COCO_API auto to_string() const noexcept -> std::string;

    /// \brief
    ///   Create a loopback ipv4 address.
    /// \return
    ///   A loopback ipv4 address.
    [[nodiscard]] static constexpr auto loopback() noexcept -> ipv4_address {
        return {127, 0, 0, 1};
    }

    /// \brief
    ///   Create a broadcast ipv4 address.
    /// \return
    ///   A broadcast ipv4 address.
    [[nodiscard]] static constexpr auto broadcast() noexcept -> ipv4_address {
        return {255, 255, 255, 255};
    }

    /// \brief
    ///   Create a ipv4 address that listen to all of the network interfaces.
    /// \return
    ///   A ipv4 address that listen to all of the network interfaces.
    [[nodiscard]] static constexpr auto any() noexcept -> ipv4_address {
        return {0, 0, 0, 0};
    }

private:
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } m_address;
};

/// \class ipv6_address
/// \brief
///     Represents an IPv6 address. This is a trivial type and it is safe to do
///     direct memory operations on ipv6_address objects.
class ipv6_address {
public:
    /// \brief
    ///   Create an empty ipv6 address. Empty ipv6 address should not be used
    ///   for any purpose.
    ipv6_address() noexcept = default;

    /// \brief
    ///   Create a new ipv6 address from raw bytes. Byte order conversion is not
    ///   performed here.
    /// \param v0
    ///   The first parameter of the ipv6 address.
    /// \param v1
    ///   The second parameter of the ipv6 address.
    /// \param v2
    ///   The third parameter of the ipv6 address.
    /// \param v3
    ///   The fourth parameter of the ipv6 address.
    /// \param v4
    ///   The fifth parameter of the ipv6 address.
    /// \param v5
    ///   The sixth parameter of the ipv6 address.
    /// \param v6
    ///   The seventh parameter of the ipv6 address.
    /// \param v7
    ///   The eighth parameter of the ipv6 address.
    COCO_API ipv6_address(uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3,
                          uint16_t v4, uint16_t v5, uint16_t v6,
                          uint16_t v7) noexcept;

    /// \brief
    ///   ipv6_address is trivially copyable.
    /// \param other
    ///   The ipv6_address to copy from.
    ipv6_address(const ipv6_address &other) noexcept = default;

    /// \brief
    ///   ipv6_address is trivially moveable.
    /// \param other
    ///   The ipv6_address to move from.
    ipv6_address(ipv6_address &&other) noexcept = default;

    /// \brief
    ///   ipv6_address is trivially destructible.
    ~ipv6_address() = default;

    /// \brief
    ///   ipv6_address is trivially copyable.
    /// \param other
    ///   The ipv6_address to copy from.
    /// \return
    ///   A reference to the ipv6_address.
    auto operator=(const ipv6_address &other) noexcept
        -> ipv6_address & = default;

    /// \brief
    ///   ipv6_address is trivially moveable.
    /// \param other
    ///   The ipv6_address to move from.
    /// \return
    ///   A reference to the ipv6_address.
    auto operator=(ipv6_address &&other) noexcept -> ipv6_address & = default;

    /// \brief
    ///   Set a new ipv6 address from string.
    /// \param address
    ///   A string that represents the ipv6 address to be set.
    /// \retval true
    ///   The ipv6 address is set successfully.
    /// \retval false
    ///   The input string is not a valid ipv6 address.
    COCO_API auto set_address(std::string_view address) noexcept -> bool;

    /// \brief
    ///   Get string representation of this ipv6 address.
    /// \return
    ///   A string that represents the ipv6 address.
    [[nodiscard]] COCO_API auto to_string() const noexcept -> std::string;

    /// \brief
    ///   Create a loopback ipv6 address.
    /// \return
    ///   A loopback ipv6 address.
    [[nodiscard]] static auto loopback() noexcept -> ipv6_address {
        return {0, 0, 0, 0, 0, 0, 0, 1};
    }

    /// \brief
    ///   Create a ipv6 address that listen to all of the network interfaces.
    /// \return
    ///   A ipv6 address that listen to all of the network interfaces.
    [[nodiscard]] static auto any() noexcept -> ipv6_address {
        return {0, 0, 0, 0, 0, 0, 0, 0};
    }

private:
    union {
        char8_t  u8[16];
        uint16_t u16[8];
        char32_t u32[4];
    } m_address;
};

/// \class ip_address
/// \brief
///     Represents an IP address. This class is a wrapper around ipv4_address
///     and ipv6_address.
class ip_address {
public:
    /// \brief
    ///   Create an empty ip_address. Empty ip_address should not be used for
    ///   any purpose.
    ip_address() noexcept = default;

    /// \brief
    ///   Create a new ip_address from ipv4_address.
    /// \param address
    ///   The ipv4_address to create the ip_address from.
    ip_address(const ipv4_address &address) noexcept
        : m_address{.v4{address}}, m_is_ipv6{false} {}

    /// \brief
    ///   Create a new ip_address from ipv6_address.
    /// \param address
    ///   The ipv6_address to create the ip_address from.
    ip_address(const ipv6_address &address) noexcept
        : m_address{.v6{address}}, m_is_ipv6{true} {}

    /// \brief
    ///   ip_address is trivially copyable.
    /// \param other
    ///   The ip_address to copy from.
    ip_address(const ip_address &other) noexcept = default;

    /// \brief
    ///   ip_address is trivially moveable.
    /// \param other
    ///   The ip_address to move from.
    ip_address(ip_address &&other) noexcept = default;

    /// \brief
    ///   ip_address is trivially destructible.
    ~ip_address() = default;

    /// \brief
    ///   ip_address is trivially copyable.
    /// \param other
    ///   The ip_address to copy from.
    /// \return
    ///   A reference to the ip_address.
    auto operator=(const ip_address &other) noexcept -> ip_address & = default;

    /// \brief
    ///   ip_address is trivially moveable.
    /// \param other
    ///   The ip_address to move from.
    /// \return
    ///   A reference to the ip_address.
    auto operator=(ip_address &&other) noexcept -> ip_address & = default;

    /// \brief
    ///   Set an ipv4_address to this ip_address.
    /// \param address
    ///   The ipv4_address to set this ip_address to.
    /// \return
    ///   A reference to this ip_address.
    auto operator=(const ipv4_address &address) noexcept -> ip_address & {
        m_address.v4 = address;
        m_is_ipv6    = false;
        return *this;
    }

    /// \brief
    ///   Set an ipv6_address to this ip_address.
    /// \param address
    ///   The ipv6_address to set this ip_address to.
    /// \return
    ///   A reference to this ip_address.
    auto operator=(const ipv6_address &address) noexcept -> ip_address & {
        m_address.v6 = address;
        m_is_ipv6    = true;
        return *this;
    }

    /// \brief
    ///   Checks if this is an ipv4 address.
    /// \retval true
    ///   This is an ipv4 address.
    /// \retval false
    ///   This is not an ipv4 address.
    [[nodiscard]] auto is_ipv4() const noexcept -> bool {
        return !m_is_ipv6;
    }

    /// \brief
    ///   Get this ip_address as an ipv4_address.
    /// \note
    ///   No check is performed to ensure that this is an ipv4 address. It is
    ///   undefined behavior if this is an ipv6 address.
    /// \return
    ///   The ipv4_address representation of this ip_address.
    [[nodiscard]] auto ipv4() noexcept -> ipv4_address & {
        assert(!m_is_ipv6);
        return m_address.v4;
    }

    /// \brief
    ///   Get this ip_address as an ipv4_address.
    /// \note
    ///   No check is performed to ensure that this is an ipv4 address. It is
    ///   undefined behavior if this is an ipv6 address.
    /// \return
    ///   The ipv4_address representation of this ip_address.
    [[nodiscard]] auto ipv4() const noexcept -> const ipv4_address & {
        assert(!m_is_ipv6);
        return m_address.v4;
    }

    /// \brief
    ///   Checks if this is an ipv6 address.
    /// \retval true
    ///   This is an ipv6 address.
    /// \retval false
    ///   This is not an ipv6 address.
    [[nodiscard]] auto is_ipv6() const noexcept -> bool {
        return m_is_ipv6;
    }

    /// \brief
    ///   Get this ip_address as an ipv6_address.
    /// \note
    ///   No check is performed to ensure that this is an ipv6 address. It is
    ///   undefined behavior if this is an ipv4 address.
    /// \return
    ///   The ipv6_address representation of this ip_address.
    [[nodiscard]] auto ipv6() noexcept -> ipv6_address & {
        assert(m_is_ipv6);
        return m_address.v6;
    }

    /// \brief
    ///   Get this ip_address as an ipv6_address.
    /// \note
    ///   No check is performed to ensure that this is an ipv6 address. It is
    ///   undefined behavior if this is an ipv4 address.
    /// \return
    ///   The ipv6_address representation of this ip_address.
    [[nodiscard]] auto ipv6() const noexcept -> const ipv6_address & {
        assert(m_is_ipv6);
        return m_address.v6;
    }

    /// \brief
    ///   Set a new ip_address from string.
    /// \param address
    ///   A string that represents the ip_address. This method will
    ///   automatically detect if the input string is an ipv4 or ipv6 address.
    /// \retval true
    ///   The ip_address is set successfully.
    /// \retval false
    ///   The input string is not a valid ip_address.
    COCO_API auto set_address(std::string_view address) noexcept -> bool;

    /// \brief
    ///   Set this ip_address as an ipv4_address.
    /// \param address
    ///   The ipv4_address to set this ip_address to.
    auto set_address(const ipv4_address &address) noexcept -> void {
        m_address.v4 = address;
        m_is_ipv6    = false;
    }

    /// \brief
    ///   Set this ip_address as an ipv6_address.
    /// \param address
    ///   The ipv6_address to set this ip_address to.
    auto set_address(const ipv6_address &address) noexcept -> void {
        m_address.v6 = address;
        m_is_ipv6    = true;
    }

    /// \brief
    ///   Get string representation of this ip_address.
    /// \return
    ///   A string that represents the ip_address.
    [[nodiscard]] COCO_API auto to_string() const noexcept -> std::string;

    /// \brief
    ///   Create an ipv4 loopback address.
    /// \return
    ///   A loopback ip_address.
    [[nodiscard]] static auto ipv4_loopback() noexcept -> ip_address {
        return {ipv4_address::loopback()};
    }

    /// \brief
    ///   Create an ipv4 broadcast address.
    /// \return
    ///   A broadcast ip_address.
    [[nodiscard]] static auto ipv4_broadcast() noexcept -> ip_address {
        return {ipv4_address::broadcast()};
    }

    /// \brief
    ///   Create an ipv4 address that listen to all of the network interfaces.
    /// \return
    ///   A ipv4 address that listen to all of the network interfaces.
    [[nodiscard]] static auto ipv4_any() noexcept -> ip_address {
        return {ipv4_address::any()};
    }

    /// \brief
    ///   Create an ipv6 loopback address.
    /// \return
    ///   A loopback ip_address.
    [[nodiscard]] static auto ipv6_loopback() noexcept -> ip_address {
        return {ipv6_address::loopback()};
    }

    /// \brief
    ///   Create an ipv6 address that listen to all of the network interfaces.
    /// \return
    ///   A ipv6 address that listen to all of the network interfaces.
    [[nodiscard]] static auto ipv6_any() noexcept -> ip_address {
        return {ipv6_address::any()};
    }

private:
    union {
        ipv4_address v4;
        ipv6_address v6;
    } m_address;
    bool m_is_ipv6;
};

/// \class tcp_connection
/// \brief
///     Represents a TCP connection. This class is a wrapper around platform
///     specific socket that supports async operations.
class tcp_connection {
public:
    /// \brief
    ///   Create an empty tcp connection. Empty tcp connection should not be
    ///   used for any purpose.
    tcp_connection() noexcept : m_socket{-1} {}

    /// \brief
    ///   For internal usage. Create a new TCP connection from socket handle.
    /// \param handle
    ///   Socket handle of this TCP connection.
    explicit tcp_connection(int handle) noexcept : m_socket{handle} {}

    /// \brief
    ///   TCP connection is not allowed to be copied.
    tcp_connection(const tcp_connection &other) = delete;

    /// \brief
    ///   Move constructor of tcp_connection.
    /// \param other
    ///   The TCP connection to move from. The moved TCP connection will be set
    ///   to empty.
    tcp_connection(tcp_connection &&other) noexcept : m_socket{other.m_socket} {
        other.m_socket = -1;
    }

    /// \brief
    ///   Disconnect and destroy this TCP connection.
    COCO_API ~tcp_connection();

    /// \brief
    ///   TCP connection is not allowed to be copied.
    auto operator=(const tcp_connection &other) = delete;

    /// \brief
    ///   Move assignment of tcp_connection.
    /// \param other
    ///   The TCP connection to move from. The moved TCP connection will be set
    ///   to empty.
    COCO_API auto operator=(tcp_connection &&other) noexcept
        -> tcp_connection &;

    /// \brief
    ///   Checks if this is an empty TCP connection.
    /// \retval true
    ///   This is an empty TCP connection.
    /// \retval false
    ///   This is not an empty TCP connection.
    [[nodiscard]] auto empty() const noexcept -> bool {
        return m_socket == -1;
    }

    /// \brief
    ///   Async connect to an endpoint. This method will release current
    ///   connection and reconnect to the new peer if current connection is not
    ///   empty.
    /// \param address
    ///   IP address of the target endpoint.
    /// \param port
    ///   Port of the target endpoint.
    /// \return
    ///   A task that connects to the target endpoint. Result of the task is a
    ///   system error code that represents the connect result. The error code
    ///   is 0 if succeeded.
    COCO_API auto connect(ip_address address, uint16_t port) noexcept
        -> connect_awaitable;

    /// \brief
    ///   Set timeout for asynchronous receive operations. See `received()` for
    ///   details.
    /// \tparam Rep
    ///   Type representation of the time type. See std::chrono::duration for
    ///   details.
    /// \tparam Period
    ///   Ratio type that is used to measure how to do conversion between
    ///   different duration types. See std::chrono::duration for details.
    /// \param duration
    ///   Timeout duration of receive operation. Nanosecond and below ratios are
    ///   not supported. Pass 0 to remove timeout events.
    /// \return
    ///   An error code that represents the result. Return 0 if succeeded to set
    ///   timeout for receive operation.
    template <class Rep, class Period>
    auto set_timeout(std::chrono::duration<Rep, Period> duration) noexcept
        -> std::error_code {
        static_assert(std::ratio_greater_equal<std::micro, Period>::value);
        return this->set_timeout(
            std::chrono::duration_cast<std::chrono::microseconds>(duration));
    }

    /// \brief
    ///   Async send data to peer.
    /// \param data
    ///   Pointer to start of data to be sent.
    /// \param size
    ///   Size in byte of data to be sent.
    /// \return
    ///   A task that sends data to the target endpoint. Result of the task is a
    ///   system error code that represents the send result. The error code is 0
    ///   if succeeded.
    auto send(const void *data, size_t size) const noexcept -> send_awaitable {
        return {m_socket, data, size, nullptr};
    }

    /// \brief
    ///   Async send data to peer.
    /// \param data
    ///   Pointer to start of data to be sent.
    /// \param size
    ///   Size in byte of data to be sent.
    /// \param[out] sent
    ///   Size in byte of data actually sent.
    /// \return
    ///   A task that sends data to the target endpoint. Result of the task is a
    ///   system error code that represents the send result. The error code is 0
    ///   if succeeded.
    auto send(const void *data, size_t size, size_t &sent) const noexcept
        -> send_awaitable {
        return {m_socket, data, size, &sent};
    }

    /// \brief
    ///   Async receive data from peer.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to receive data.
    /// \param size
    ///   Maximum available size to be received.
    /// \return
    ///   A task that receives data to the target endpoint. Result of the task
    ///   is a system error code that represents the receive result. The error
    ///   code is 0 if succeeded. The error code is `EAGAIN` if timeout occurs.
    auto receive(void *buffer, size_t size) const noexcept -> recv_awaitable {
        return {m_socket, buffer, size, nullptr};
    }

    /// \brief
    ///   Async receive data from peer.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to receive data.
    /// \param size
    ///   Maximum available size to be received.
    /// \param[out] received
    ///   Size in byte of data actually received.
    /// \return
    ///   A task that receives data to the target endpoint. Result of the task
    ///   is a system error code that represents the receive result. The error
    ///   code is 0 if succeeded. The error code is `EAGAIN` if timeout occurs.
    auto receive(void *buffer, size_t size, size_t &received) const noexcept
        -> recv_awaitable {
        return {m_socket, buffer, size, &received};
    }

    /// \brief
    ///   Shutdown write pipe of this TCP connection.
    /// \return
    ///   System error code of this operation. The error code is 0 if succeeded.
    COCO_API auto shutdown() noexcept -> std::error_code;

    /// \brief
    ///   Close this TCP connection.
    COCO_API auto close() noexcept -> void;

    /// \brief
    ///   Enable or disable keepalive for this TCP connection.
    /// \param enable
    ///   True to enable keepalive. False to disable it.
    /// \return
    ///   System error code of this operation. The error code is 0 if succeeded.
    COCO_API auto keepalive(bool enable) noexcept -> std::error_code;

    /// \brief
    ///   Get peer IP address.
    /// \return
    ///   The IP address of the peer.
    [[nodiscard]] COCO_API auto address() const noexcept -> ip_address;

    /// \brief
    ///   Get peer port number.
    /// \return
    ///   The port number of the peer.
    [[nodiscard]] COCO_API auto port() const noexcept -> uint16_t;

private:
    /// \brief
    ///   For internal usage. Set timeout for asynchronous receive operations.
    ///   The `received` parameter will be set to 0 if timeout occurs.
    /// \param microseconds
    ///   Microseconds of the receive timeout duration. Pass 0 to remove the
    ///   timeout event.
    /// \return
    ///   An error code that represents the result. Return 0 if succeeded to set
    ///   timeout for receive operation.
    COCO_API auto set_timeout(int64_t microseconds) noexcept -> std::error_code;

private:
    int m_socket;
};

/// \class tcp_server
/// \brief
///     TCP server class. TCP server listens to a specific port and accepts
///     incoming connections.
class tcp_server {
public:
    /// \class accept_awaitable
    /// \brief
    ///   For internal usage. Accept awaitable for TCP connections.
    class accept_awaitable {
    public:
        /// \brief
        ///   For internal usage. Create a new accept awaitable object to
        ///   prepare for async accept operation.
        /// \param socket
        ///   TCP server socket handle.
        /// \param[out] connection
        accept_awaitable(int socket, tcp_connection *connection) noexcept
            : m_userdata{}, m_socket{socket}, m_socklen{sizeof(m_sockaddr)},
              m_sockaddr{}, m_connection{connection} {}

        /// \brief
        ///   C++20 awaitable internal method. Always execute await_suspend().
        auto await_ready() noexcept -> bool {
            return false;
        }

        /// \brief
        ///   Prepare async read operation and suspend this coroutine.
        /// \tparam Promise
        ///   Type of the promise for the coroutine.
        /// \param coro
        ///   Current coroutine handle.
        /// \return
        ///   A boolean value that specifies whether to suspend current
        ///   coroutine. Current coroutine will be suspended if no error occurs.
        template <class Promise>
        auto await_suspend(std::coroutine_handle<Promise> coro) noexcept
            -> bool {
            m_userdata.coroutine = coro.address();
            return this->suspend(coro.promise().worker());
        }

        /// \brief
        ///   Resume current coroutine and get result of the async accept
        ///   operation.
        /// \return
        ///   Error code of the async accept operation. The error code is 0 if
        ///   no error occurs.
        auto await_resume() noexcept -> std::error_code {
            if (m_userdata.cqe_res < 0)
                return {-m_userdata.cqe_res, std::system_category()};
            assert(m_connection != nullptr);
            *m_connection = tcp_connection{m_userdata.cqe_res};
            return {};
        }

    private:
        /// \brief
        ///   For internal usage. Actually prepare async accept operation and
        ///   suspend this coroutine.
        /// \param[in] worker
        ///   Worker for current thread.
        COCO_API auto suspend(detail::io_context_worker *worker) noexcept
            -> bool;

    private:
        detail::user_data m_userdata;
        int               m_socket;
        socklen_t         m_socklen;
        sockaddr_storage  m_sockaddr;
        tcp_connection   *m_connection;
    };

public:
    /// \brief
    ///   Create an empty TCP server.
    tcp_server() noexcept : m_address{}, m_port{}, m_socket{-1} {}

    /// \brief
    ///   TCP server is not allowed to be copied.
    tcp_server(const tcp_server &other) = delete;

    /// \brief
    ///   Move constructor of TCP server.
    /// \param other
    ///   The TCP server to be moved from. The moved TCP server will be set to
    ///   empty.
    tcp_server(tcp_server &&other) noexcept
        : m_address{other.m_address}, m_port{other.m_port},
          m_socket{other.m_socket} {
        other.m_socket = -1;
    }

    /// \brief
    ///   Destroy this TCP server.
    COCO_API ~tcp_server();

    /// \brief
    ///   TCP server is not allowed to be copied.
    auto operator=(const tcp_server &other) = delete;

    /// \brief
    ///   Move assignment of TCP server.
    /// \param other
    ///   The TCP server to be moved from. The moved TCP server will be set to
    ///   empty.
    /// \return
    ///   Reference to this TCP server.
    COCO_API auto operator=(tcp_server &&other) noexcept -> tcp_server &;

    /// \brief
    ///   Bind this TCP server to the specified address and start listening to
    ///   connections.
    /// \param address
    ///   The IP address that this TCP server to bind.
    /// \param port
    ///   The local port that this TCP server to bind.
    /// \return
    ///   An error code that represents the listen result. The error code is 0
    ///   if succeeded to listen to the specified address and port.
    COCO_API auto listen(const ip_address &address, uint16_t port) noexcept
        -> std::error_code;

    /// \brief
    ///   Try to accept a new TCP connection asynchronously.
    /// \param[out] connection
    ///   The TCP connection object that is used to store the new connection.
    /// \return
    ///   A task that receives connection from the target endpoint. Result of
    ///   the task is a system error code that represents the accept result. The
    ///   error code is 0 if succeeded.
    auto accept(tcp_connection &connection) noexcept -> accept_awaitable {
        return {m_socket, &connection};
    }

    /// \brief
    ///   Close this TCP server.
    COCO_API auto close() noexcept -> void;

    /// \brief
    ///   Get local IP address.
    /// \return
    ///   The listening IP address. The return value is undefined if `listen()`
    ///   is not called.
    [[nodiscard]] auto address() const noexcept -> const ip_address & {
        return m_address;
    }

    /// \brief
    ///   Get local port number.
    /// \return
    ///   The listening port. The return value is undefined if `listen()` is not
    ///   called.
    [[nodiscard]] auto port() const noexcept -> uint16_t {
        return m_port;
    }

private:
    ip_address m_address;
    uint16_t   m_port;
    int        m_socket;
};

/// \class binary_file
/// \brief
///   Wrapper class for binary file. This class provides async IO utilities for
///   binary file.
class binary_file {
public:
    /// \brief
    ///   Open file flags. File flags supports binary operations.
    enum class flag : uint32_t {
        // Open for reading.
        read = 0x0001,
        // Open for writing. A new file will be created if not exist.
        write = 0x0002,
        // The file is opened in append mode. Before each write(2), the file
        // offset is positioned at the end of the file.
        append = 0x0004,
        // The file will be truncated to length 0 once opened. Must be used with
        // flag::write.
        trunc = 0x0008,
    };

    /// \brief
    ///   Specifies the file seeking direction type.
    enum class seek_whence : uint16_t {
        begin   = 0,
        current = 1,
        end     = 2,
    };

    /// \brief
    ///   Create an empty binary file object. Empty binary file object cannot be
    ///   used for IO.
    binary_file() noexcept : m_file{-1} {}

    /// \brief
    ///   Binary file is not allowed to be copied.
    binary_file(const binary_file &other) = delete;

    /// \brief
    ///   Move constructor of binary file.
    /// \param other
    ///   The binary file object to be moved. The moved binary file will be set
    ///   empty.
    binary_file(binary_file &&other) noexcept : m_file{other.m_file} {
        other.m_file = -1;
    }

    /// \brief
    ///   Close this binary file and destroy this object.
    COCO_API ~binary_file();

    /// \brief
    ///   Binary file is not allowed to be copied.
    auto operator=(const binary_file &other) = delete;

    /// \brief
    ///   Move assignment of binary file.
    /// \param other
    ///   The binary file object to be moved. The moved binary file will be set
    ///   empty.
    /// \return
    ///   Reference to this binary file object.
    COCO_API auto operator=(binary_file &&other) noexcept -> binary_file &;

    /// \brief
    ///   Checks if this file is opened.
    /// \retval true
    ///   This file is opened.
    /// \retval false
    ///   This file is not opened.
    [[nodiscard]] auto is_open() const noexcept -> bool {
        return m_file != -1;
    }

    /// \brief
    ///   Get size in byte of this file.
    /// \warning
    ///   Errors are not handled. This method may be changed in the future.
    /// \return
    ///   Size in byte of this file.
    [[nodiscard]] COCO_API auto size() noexcept -> size_t;

    /// \brief
    ///   Try to open or reopen a file.
    /// \param path
    ///   Path of the file to be opened.
    /// \param flags
    ///   Open file flags. See binary_file::flag for details.
    /// \return
    ///   A system error code that represents the open result. Return 0 if
    ///   succeeded. This method does not modify this object if failed.
    COCO_API auto open(std::string_view path, flag flags) noexcept
        -> std::error_code;

    /// \brief
    ///   Async read data from this file.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to read data.
    /// \param size
    ///   Expected size of data to be read.
    /// \return
    ///   A task that reads data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto read(void *buffer, uint32_t size) noexcept -> read_awaitable {
        return {m_file, buffer, size, nullptr, uint64_t(-1)};
    }

    /// \brief
    ///   Async read data from this file.
    /// \param offset
    ///   Offset of the file to read from. This offset does not affect the
    ///   internal seek pointer.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to read data.
    /// \param size
    ///   Expected size of data to be read.
    /// \return
    ///   A task that reads data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto read(size_t offset, void *buffer, uint32_t size) noexcept
        -> read_awaitable {
        return {m_file, buffer, size, nullptr, offset};
    }

    /// \brief
    ///   Async read data from this file.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to read data.
    /// \param size
    ///   Expected size of data to be read.
    /// \param[out] bytes
    ///   Size in byte of data actually read.
    /// \return
    ///   A task that reads data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto read(void *buffer, uint32_t size, uint32_t &bytes) noexcept
        -> read_awaitable {
        return {m_file, buffer, size, &bytes, uint64_t(-1)};
    }

    /// \brief
    ///   Async read data from this file.
    /// \param offset
    ///   Offset of the file to read from. This offset does not affect the
    ///   internal seek pointer.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to read data.
    /// \param size
    ///   Expected size of data to be read.
    /// \param[out] bytes
    ///   Size in byte of data actually read.
    /// \return
    ///   A task that reads data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto read(size_t offset, void *buffer, uint32_t size,
              uint32_t &bytes) noexcept -> read_awaitable {
        return {m_file, buffer, size, &bytes, offset};
    }

    /// \brief
    ///   Async write data to this file.
    /// \param data
    ///   Pointer to start of data to be written.
    /// \param size
    ///   Expected size in byte of data to be written.
    /// \return
    ///   A task that writes data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto write(const void *data, uint32_t size) noexcept -> write_awaitable {
        return {m_file, data, size, nullptr, uint64_t(-1)};
    }

    /// \brief
    ///   Async write data to this file.
    /// \param offset
    ///   Offset of file to start writing data. This offset does not affect the
    ///   internal seek pointer.
    /// \param data
    ///   Pointer to start of data to be written.
    /// \param size
    ///   Expected size in byte of data to be written.
    /// \return
    ///   A task that writes data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto write(size_t offset, const void *data, uint32_t size) noexcept
        -> write_awaitable {
        return {m_file, data, size, nullptr, offset};
    }

    /// \brief
    ///   Async write data to this file.
    /// \param data
    ///   Pointer to start of data to be written.
    /// \param size
    ///   Expected size in byte of data to be written.
    /// \param[out] bytes
    ///   Size in byte of data actually written.
    /// \return
    ///   A task that writes data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto write(const void *data, uint32_t size, uint32_t &bytes) noexcept
        -> write_awaitable {
        return {m_file, data, size, &bytes, uint64_t(-1)};
    }

    /// \brief
    ///   Async write data to this file.
    /// \param offset
    ///   Offset of file to start writing data. This offset does not affect the
    ///   internal seek pointer.
    /// \param data
    ///   Pointer to start of data to be written.
    /// \param size
    ///   Expected size in byte of data to be written.
    /// \param[out] bytes
    ///   Size in byte of data actually written.
    /// \return
    ///   A task that writes data from this file. Result of the task is a system
    ///   error code that represents the receive result. The error code is 0 if
    ///   succeeded.
    auto write(size_t offset, const void *data, uint32_t size,
               uint32_t &bytes) noexcept -> write_awaitable {
        return {m_file, data, size, &bytes, offset};
    }

    /// \brief
    ///   Seek internal pointer of this file.
    /// \param whence
    ///   Specifies the file seeking direction type.
    /// \param offset
    ///   Offset of the internal IO pointer.
    /// \return
    ///   An error code that represents the seek result. Return 0 if succeeded.
    COCO_API auto seek(seek_whence whence, ptrdiff_t offset) noexcept
        -> std::error_code;

    /// \brief
    ///   Flush data and metadata of this file.
    auto flush() noexcept -> void {
        if (m_file != -1)
            ::fsync(m_file);
    }

    /// \brief
    ///   Close this binary file.
    auto close() noexcept -> void {
        if (m_file != -1) {
            ::close(m_file);
            m_file = -1;
        }
    }

private:
    int m_file;
};

constexpr auto operator~(binary_file::flag flag) noexcept -> binary_file::flag {
    return static_cast<binary_file::flag>(~static_cast<uint32_t>(flag));
}

constexpr auto operator|(binary_file::flag lhs, binary_file::flag rhs) noexcept
    -> binary_file::flag {
    return static_cast<binary_file::flag>(static_cast<uint32_t>(lhs) |
                                          static_cast<uint32_t>(rhs));
}

constexpr auto operator&(binary_file::flag lhs, binary_file::flag rhs) noexcept
    -> binary_file::flag {
    return static_cast<binary_file::flag>(static_cast<uint32_t>(lhs) &
                                          static_cast<uint32_t>(rhs));
}

constexpr auto operator^(binary_file::flag lhs, binary_file::flag rhs) noexcept
    -> binary_file::flag {
    return static_cast<binary_file::flag>(static_cast<uint32_t>(lhs) ^
                                          static_cast<uint32_t>(rhs));
}

constexpr auto operator|=(binary_file::flag &lhs,
                          binary_file::flag  rhs) noexcept
    -> binary_file::flag & {
    lhs = (lhs | rhs);
    return lhs;
}

constexpr auto operator&=(binary_file::flag &lhs,
                          binary_file::flag  rhs) noexcept
    -> binary_file::flag & {
    lhs = (lhs & rhs);
    return lhs;
}

constexpr auto operator^=(binary_file::flag &lhs,
                          binary_file::flag  rhs) noexcept
    -> binary_file::flag & {
    lhs = (lhs ^ rhs);
    return lhs;
}

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
        -> timeout_awaitable {
        static_assert(std::ratio_less_equal_v<std::nano, Period>,
                      "The maximum available precision is nanosecond.");
        std::chrono::nanoseconds nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        return {m_timer, nanoseconds.count()};
    }

private:
    int m_timer;
};

} // namespace coco

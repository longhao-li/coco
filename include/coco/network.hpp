#pragma once

#include "task.hpp"

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
    tcp_connection() noexcept : m_address{}, m_port{}, m_socket{-1} {}

    /// \brief
    ///   TCP connection is not allowed to be copied.
    tcp_connection(const tcp_connection &other) = delete;

    /// \brief
    ///   Move constructor of tcp_connection.
    /// \param other
    ///   The TCP connection to move from. The moved TCP connection will be set
    ///   to empty.
    tcp_connection(tcp_connection &&other) noexcept
        : m_address{other.m_address}, m_port{other.m_port},
          m_socket{other.m_socket} {
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
        -> task<std::error_code>;

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
    COCO_API auto send(const void *data, size_t size) const noexcept
        -> task<std::error_code>;

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
    COCO_API auto send(const void *data, size_t size,
                       size_t &sent) const noexcept -> task<std::error_code>;

    /// \brief
    ///   Async receive data from peer.
    /// \param[out] buffer
    ///   Pointer to start of the buffer to receive data.
    /// \param size
    ///   Maximum available size to be received.
    /// \return
    ///   A task that receives data to the target endpoint. Result of the task
    ///   is a system error code that represents the receive result. The error
    ///   code is 0 if succeeded.
    COCO_API auto receive(void *buffer, size_t size) const noexcept
        -> task<std::error_code>;

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
    ///   code is 0 if succeeded.
    COCO_API auto receive(void *buffer, size_t size,
                          size_t &received) const noexcept
        -> task<std::error_code>;

    /// \brief
    ///   Shutdown write pipe of this TCP connection.
    /// \return
    ///   System error code of this operation. The error code is 0 if succeeded.
    COCO_API auto shutdown() noexcept -> std::error_code;

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
    [[nodiscard]] auto address() const noexcept -> const ip_address & {
        return m_address;
    }

    /// \brief
    ///   Get peer port number.
    /// \return
    ///   The port number of the peer.
    [[nodiscard]] auto port() const noexcept -> uint16_t {
        return m_port;
    }

    friend class tcp_server;

private:
    /// \brief
    ///   For internal usage. Create a new TCP connection with the given
    ///   address, port and socket.
    /// \param address
    ///   IP address of the peer.
    /// \param port
    ///   Port number of the peer.
    /// \param socket
    ///   Platform specific socket handle.
    tcp_connection(const ip_address &address, uint16_t port,
                   int socket) noexcept
        : m_address{address}, m_port{port}, m_socket{socket} {}

private:
    ip_address m_address;
    uint16_t   m_port;
    int        m_socket;
};

/// \class tcp_server
/// \brief
///     TCP server class. TCP server listens to a specific port and accepts
///     incoming connections.
class tcp_server {
public:
    /// \brief
    ///   Create an empty TCP server.
    COCO_API tcp_server() noexcept
        : m_address{}, m_port{}, m_socket{-1}, m_should_exit{false},
          m_is_looping{false} {}

    /// \brief
    ///   TCP server is not allowed to be copied.
    tcp_server(const tcp_server &other) = delete;

    /// \brief
    ///   TCP server is not allowed to be moved.
    tcp_server(tcp_server &&other) = delete;

    /// \brief
    ///   Destroy this TCP server.
    COCO_API virtual ~tcp_server();

    /// \brief
    ///   TCP server is not allowed to be copied.
    auto operator=(const tcp_server &other) = delete;

    /// \brief
    ///   TCP server is not allowed to be moved.
    auto operator=(tcp_server &&other) = delete;

    /// \brief
    ///   Bind this TCP server to the specified address and start listening to
    ///   connections.
    /// \param address
    ///   The IP address that this TCP server to bind.
    /// \param port
    ///   The local port that this TCP server to bind.
    COCO_API auto listen(const ip_address &address, uint16_t port) noexcept
        -> std::error_code;

    /// \brief
    ///   Start running and accepting income TCP connections. This method will
    ///   block current thread.
    /// \param[in] io_ctx
    ///   The IO context that is used to handle TCP connections.
    COCO_API auto run(io_context &io_ctx) noexcept -> void;

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

    /// \brief
    ///   Callback method that is called when a new connection is accepted.
    ///   Override this method to support customize behavior.
    /// \param connection
    ///   The new connection that is accepted.
    /// \return
    ///   A task that represents the asynchronous method to be called. Default
    ///   implementation returns a task that does nothing.
    COCO_API virtual auto on_accept(tcp_connection connection) noexcept
        -> task<void>;

private:
    ip_address       m_address;
    uint16_t         m_port;
    int              m_socket;
    std::atomic_bool m_should_exit;
    std::atomic_bool m_is_looping;
};

} // namespace coco

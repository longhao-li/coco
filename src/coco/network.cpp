#include "coco/network.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

using namespace coco;

auto coco::ipv4_address::set_address(std::string_view address) noexcept
    -> bool {
    char buffer[INET_ADDRSTRLEN];
    if (address.size() >= INET_ADDRSTRLEN)
        return false;

    memcpy(buffer, address.data(), address.size());
    buffer[address.size()] = '\0';

    in_addr binary;
    if (inet_pton(AF_INET, buffer, &binary) != 1)
        return false;

    static_assert(sizeof(in_addr) == sizeof(m_address));
    memcpy(&m_address, &binary, sizeof(m_address));

    return true;
}

auto coco::ipv4_address::to_string() const noexcept -> std::string {
    char buffer[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &m_address, buffer, sizeof(buffer));
    return buffer;
}

coco::ipv6_address::ipv6_address(uint16_t v0, uint16_t v1, uint16_t v2,
                                 uint16_t v3, uint16_t v4, uint16_t v5,
                                 uint16_t v6, uint16_t v7) noexcept
    : m_address{.u16{ntohs(v0), ntohs(v1), ntohs(v2), ntohs(v3), ntohs(v4),
                     ntohs(v5), ntohs(v6), ntohs(v7)}} {}

auto coco::ipv6_address::set_address(std::string_view address) noexcept
    -> bool {
    char buffer[INET6_ADDRSTRLEN];
    if (address.size() >= INET6_ADDRSTRLEN)
        return false;

    memcpy(buffer, address.data(), address.size());
    buffer[address.size()] = '\0';

    in6_addr binary;
    if (inet_pton(AF_INET6, buffer, &binary) != 1)
        return false;

    static_assert(sizeof(binary) == sizeof(m_address));
    memcpy(&m_address, &binary, sizeof(m_address));

    return true;
}

auto coco::ipv6_address::to_string() const noexcept -> std::string {
    char buffer[INET6_ADDRSTRLEN]{};
    inet_ntop(AF_INET6, &m_address, buffer, sizeof(buffer));
    return buffer;
}

auto coco::ip_address::set_address(std::string_view address) noexcept -> bool {
    if (address.find(':') != std::string_view::npos) {
        ipv6_address new_address;
        if (!new_address.set_address(address))
            return false;

        m_address.v6 = new_address;
        m_is_ipv6    = true;
    } else {
        ipv4_address new_address;
        if (!new_address.set_address(address))
            return false;

        m_address.v4 = new_address;
        m_is_ipv6    = false;
    }

    return true;
}

auto coco::ip_address::to_string() const noexcept -> std::string {
    if (m_is_ipv6)
        return m_address.v6.to_string();
    return m_address.v4.to_string();
}

coco::tcp_connection::~tcp_connection() {
    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
    }
}

auto coco::tcp_connection::operator=(tcp_connection &&other) noexcept
    -> tcp_connection & {
    if (this == &other) [[unlikely]]
        return *this;

    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
    }

    m_address = other.m_address;
    m_port    = other.m_port;
    m_socket  = other.m_socket;

    other.m_socket = -1;
    return *this;
}

namespace {

struct connect_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int       m_socket;
    sockaddr *m_sockaddr;
    socklen_t m_socklen;

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

        io_uring_prep_connect(sqe, m_socket, m_sockaddr, m_socklen);
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

auto coco::tcp_connection::connect(ip_address address, uint16_t port) noexcept
    -> task<std::error_code> {
    sockaddr_storage temp{};
    socklen_t        length;

    if (address.is_ipv4()) {
        auto *sock_v4 = reinterpret_cast<sockaddr_in *>(&temp);
        length        = sizeof(sockaddr_in);

        sock_v4->sin_family = AF_INET;
        sock_v4->sin_port   = htons(port);
        memcpy(&sock_v4->sin_addr, &address.ipv4(), sizeof(in_addr));
    } else {
        auto *sock_v6 = reinterpret_cast<sockaddr_in6 *>(&temp);
        length        = sizeof(sockaddr_in6);

        sock_v6->sin6_family = AF_INET6;
        sock_v6->sin6_port   = htons(port);
        memcpy(&sock_v6->sin6_addr, &address.ipv6(), sizeof(in6_addr));
    }

    int new_socket = socket(temp.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket < 0) [[unlikely]]
        co_return std::error_code{errno, std::system_category()};

    connect_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = new_socket,
        .m_sockaddr = reinterpret_cast<sockaddr *>(&temp),
        .m_socklen  = length,
    };

    int result = co_await awaitable;
    if (result < 0) {
        ::close(new_socket);
        co_return std::error_code{-result, std::system_category()};
    }

    if (m_socket != -1)
        ::close(m_socket);

    m_address = address;
    m_port    = port;
    m_socket  = new_socket;

    co_return std::error_code{};
}

namespace {

struct send_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int         m_socket;
    const void *m_data;
    size_t      m_size;

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

        io_uring_prep_send(sqe, m_socket, m_data, m_size, MSG_NOSIGNAL);
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

auto coco::tcp_connection::send(const void *data, size_t size) const noexcept
    -> task<std::error_code> {
    send_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = m_socket,
        .m_data     = data,
        .m_size     = size,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::tcp_connection::send(const void *data, size_t size,
                                size_t &sent) const noexcept
    -> task<std::error_code> {
    send_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = m_socket,
        .m_data     = data,
        .m_size     = size,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    sent = static_cast<size_t>(result);
    co_return std::error_code{};
}

namespace {

struct receive_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int    m_socket;
    void  *m_buffer;
    size_t m_size;

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

        io_uring_prep_recv(sqe, m_socket, m_buffer, m_size, 0);
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

auto coco::tcp_connection::receive(void *buffer, size_t size) const noexcept
    -> task<std::error_code> {
    receive_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = m_socket,
        .m_buffer   = buffer,
        .m_size     = size,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::tcp_connection::receive(void *buffer, size_t size,
                                   size_t &received) const noexcept
    -> task<std::error_code> {
    receive_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = m_socket,
        .m_buffer   = buffer,
        .m_size     = size,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    received = static_cast<size_t>(result);
    co_return std::error_code{};
}

auto coco::tcp_connection::shutdown() noexcept -> std::error_code {
    if (::shutdown(m_socket, SHUT_WR) < 0)
        return {errno, std::system_category()};
    return {};
}

auto coco::tcp_connection::close() noexcept -> void {
    if (m_socket != -1) {
        ::close(m_socket);
        m_socket = -1;
    }
}

auto coco::tcp_connection::keepalive(bool enable) noexcept -> std::error_code {
    int value = enable ? 1 : 0;
    int result =
        setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
    if (result == -1)
        return std::error_code{errno, std::system_category()};
    return std::error_code{};
}

auto coco::tcp_connection::set_timeout(int64_t microseconds) noexcept
    -> std::error_code {
    timeval timeout{
        .tv_sec  = microseconds / 1000000,
        .tv_usec = microseconds % 1000000,
    };

    int result = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                            sizeof(timeout));
    if (result == -1)
        return std::error_code{errno, std::system_category()};
    return std::error_code{};
}

coco::tcp_server::~tcp_server() {
    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
    }
}

auto coco::tcp_server::listen(const ip_address &address, uint16_t port) noexcept
    -> std::error_code {
    sockaddr_storage temp{};
    socklen_t        length;

    if (address.is_ipv4()) {
        auto *sock_v4 = reinterpret_cast<sockaddr_in *>(&temp);
        length        = sizeof(sockaddr_in);

        sock_v4->sin_family = AF_INET;
        sock_v4->sin_port   = htons(port);
        memcpy(&sock_v4->sin_addr, &address.ipv4(), sizeof(in_addr));
    } else {
        auto *sock_v6 = reinterpret_cast<sockaddr_in6 *>(&temp);
        length        = sizeof(sockaddr_in6);

        sock_v6->sin6_family = AF_INET6;
        sock_v6->sin6_port   = htons(port);
        memcpy(&sock_v6->sin6_addr, &address.ipv6(), sizeof(in6_addr));
    }

    int new_socket = socket(temp.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket < 0)
        return {errno, std::system_category()};

    int result = bind(new_socket, reinterpret_cast<sockaddr *>(&temp), length);
    if (result < 0) {
        int error = errno;
        ::close(new_socket);
        return {error, std::system_category()};
    }

    result = ::listen(new_socket, SOMAXCONN);
    if (result < 0) {
        int error = errno;
        ::close(new_socket);
        return {error, std::system_category()};
    }

    if (m_socket != -1)
        ::close(m_socket);

    m_address = address;
    m_port    = port;
    m_socket  = new_socket;

    // Set reuse address and port.
    int enable = 1;
    setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    enable = 1;
    setsockopt(new_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    return {};
}

namespace {

struct accept_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int              m_socket;
    sockaddr_storage m_sockaddr;
    socklen_t        m_socklen;

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

        m_socklen = sizeof(m_sockaddr);
        io_uring_prep_accept(sqe, m_socket,
                             reinterpret_cast<sockaddr *>(&m_sockaddr),
                             &m_socklen, SOCK_NONBLOCK | SOCK_CLOEXEC);
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

auto coco::tcp_server::accept(tcp_connection &connection) noexcept
    -> task<std::error_code> {
    assert(m_socket != -1);
    accept_awaitable awaitable{
        .m_userdata = {},
        .m_socket   = m_socket,
        .m_sockaddr = {},
        .m_socklen  = 0,
    };

    int peer = co_await awaitable;
    if (peer < 0) [[unlikely]]
        co_return std::error_code{-peer, std::system_category()};

    sockaddr_storage *temp = &awaitable.m_sockaddr;

    int        family = temp->ss_family;
    ip_address address;
    uint16_t   port = 0;

    if (family == AF_INET) {
        auto        *v4 = reinterpret_cast<sockaddr_in *>(temp);
        ipv4_address v4_addr;

        port = ntohs(v4->sin_port);
        memcpy(&v4_addr, &v4->sin_addr, sizeof(in_addr));
        address = v4_addr;
    } else {
        assert(family == AF_INET6);
        auto        *v6 = reinterpret_cast<sockaddr_in6 *>(temp);
        ipv6_address v6_addr;

        port = ntohs(v6->sin6_port);
        memcpy(&v6_addr, &v6->sin6_addr, sizeof(in6_addr));
        address = v6_addr;
    }

    connection = tcp_connection{address, port, peer};
    co_return std::error_code{};
}

auto coco::tcp_server::close() noexcept -> void {
    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
        m_socket = -1;
    }
}

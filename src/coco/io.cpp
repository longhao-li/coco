#if defined(__LP64__)
#    define _FILE_OFFSET_BITS 64
#endif
#include "coco/io.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cstring>

using namespace coco;
using namespace coco::detail;

auto coco::read_awaitable::suspend(io_context_worker *worker) noexcept -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_read(sqe, m_file, m_buffer, m_size, m_offset);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

auto coco::write_awaitable::suspend(io_context_worker *worker) noexcept
    -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_write(sqe, m_file, m_data, m_size, m_offset);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

auto coco::connect_awaitable::await_resume() noexcept -> std::error_code {
    if (m_userdata.cqe_res < 0)
        return {-m_userdata.cqe_res, std::system_category()};

    assert(m_target != nullptr);
    if (*m_target >= 0)
        ::close(*m_target);

    *m_target = m_socket;
    return {};
}

auto coco::connect_awaitable::suspend(
    detail::io_context_worker *worker) noexcept -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    m_socket = ::socket(m_sockaddr.ss_family, m_type, m_protocol);
    if (m_socket < 0) { // Failed to create new socket.
        m_userdata.cqe_res = -errno;
        return false;
    }

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_connect(sqe, m_socket,
                          reinterpret_cast<sockaddr *>(&m_sockaddr), m_socklen);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        ::close(m_socket);
        return false;
    }

    return true;
}

auto coco::send_awaitable::suspend(io_context_worker *worker) noexcept -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_send(sqe, m_socket, m_data, m_size, MSG_NOSIGNAL);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

auto coco::recv_awaitable::suspend(io_context_worker *worker) noexcept -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_recv(sqe, m_socket, m_buffer, m_size, 0);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

auto coco::timeout_awaitable::suspend(io_context_worker *worker) noexcept
    -> bool {
    itimerspec timeout{
        .it_interval = {},
        .it_value =
            {
                .tv_sec  = m_nanosecond / 1000000000,
                .tv_nsec = m_nanosecond % 1000000000,
            },
    };

    int result = timerfd_settime(m_timer, 0, &timeout, nullptr);
    if (result == -1) { // Failed to set timer.
        m_userdata.cqe_res = -errno;
        return false;
    }

    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_read(sqe, m_timer, &m_buffer, sizeof(m_buffer), 0);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

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

    m_socket       = other.m_socket;
    other.m_socket = -1;

    return *this;
}

auto coco::tcp_connection::connect(ip_address address, uint16_t port) noexcept
    -> connect_awaitable {
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

    return {&m_socket, temp, length, SOCK_STREAM, IPPROTO_TCP};
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

auto coco::tcp_connection::address() const noexcept -> ip_address {
    sockaddr_storage addr{};
    socklen_t        length = sizeof(addr);

    int result =
        getpeername(m_socket, reinterpret_cast<sockaddr *>(&addr), &length);
    assert(result != -1);
    (void)result;

    if (addr.ss_family == AF_INET) {
        assert(length == sizeof(sockaddr_in));
        ipv4_address address;
        sockaddr_in *v4 = reinterpret_cast<sockaddr_in *>(&addr);
        memcpy(&address, &v4->sin_addr, sizeof(in_addr));
        return address;
    } else {
        assert(addr.ss_family == AF_INET6);
        assert(length == sizeof(sockaddr_in6));
        ipv6_address  address;
        sockaddr_in6 *v6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        memcpy(&address, &v6->sin6_addr, sizeof(in6_addr));
        return address;
    }
}

auto coco::tcp_connection::port() const noexcept -> uint16_t {
    sockaddr_storage addr{};
    socklen_t        length = sizeof(addr);

    int result =
        getpeername(m_socket, reinterpret_cast<sockaddr *>(&addr), &length);
    assert(result != -1);
    (void)result;

    if (addr.ss_family == AF_INET) {
        assert(length == sizeof(sockaddr_in));
        sockaddr_in *v4 = reinterpret_cast<sockaddr_in *>(&addr);
        return ntohs(v4->sin_port);
    } else {
        assert(addr.ss_family == AF_INET6);
        assert(length == sizeof(sockaddr_in6));
        sockaddr_in6 *v6 = reinterpret_cast<sockaddr_in6 *>(&addr);
        return ntohs(v6->sin6_port);
    }
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

auto coco::tcp_server::accept_awaitable::suspend(
    detail::io_context_worker *worker) noexcept -> bool {
    assert(worker != nullptr);
    m_userdata.cqe_res   = 0;
    m_userdata.cqe_flags = 0;

    io_uring     *ring = worker->io_ring();
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);
    while (sqe == nullptr) [[unlikely]] {
        io_uring_submit(ring);
        sqe = io_uring_get_sqe(ring);
    }

    io_uring_prep_accept(sqe, m_socket,
                         reinterpret_cast<sockaddr *>(&m_sockaddr), &m_socklen,
                         SOCK_CLOEXEC);
    sqe->user_data = reinterpret_cast<uint64_t>(&m_userdata);

    int result = io_uring_submit(ring);
    if (result < 0) [[unlikely]] { // Result is -errno.
        m_userdata.cqe_res = result;
        return false;
    }

    return true;
}

coco::tcp_server::~tcp_server() {
    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
    }
}

auto coco::tcp_server::listen(const ip_address &address,
                              uint16_t port) noexcept -> std::error_code {
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

auto coco::tcp_server::close() noexcept -> void {
    if (m_socket != -1) {
        assert(m_socket >= 0);
        ::close(m_socket);
        m_socket = -1;
    }
}

coco::binary_file::~binary_file() {
    if (m_file != -1) {
        assert(m_file >= 0);
        ::close(m_file);
    }
}

auto coco::binary_file::operator=(binary_file &&other) noexcept
    -> binary_file & {
    if (this == &other) [[unlikely]]
        return *this;

    if (m_file != -1)
        ::close(m_file);

    m_file       = other.m_file;
    other.m_file = -1;

    return *this;
}

auto coco::binary_file::size() noexcept -> size_t {
    struct stat info {};

    // FIXME: Report error.
    int result = fstat(m_file, &info);
    assert(result >= 0);
    (void)result;

    return static_cast<size_t>(info.st_size);
}

auto coco::binary_file::open(std::string_view path,
                             flag flags) noexcept -> std::error_code {
    char buffer[PATH_MAX];
    if (path.size() >= PATH_MAX)
        return std::error_code{EINVAL, std::system_category()};

    memcpy(buffer, path.data(), path.size());
    buffer[path.size()] = '\0';

    int unix_flag = O_DIRECT | O_CLOEXEC;
    int unix_mode = 0;
    if ((flags & flag::write) == flag::write) {
        unix_flag |= O_CREAT;
        if ((flags & flag::read) == flag::read)
            unix_flag |= O_RDWR;
        else
            unix_flag |= O_WRONLY;

        if ((flags & flag::append) == flag::append)
            unix_flag |= O_APPEND;
        if ((flags & flag::trunc) == flag::trunc)
            unix_flag |= O_TRUNC;

        unix_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    } else if ((flags & flag::read) == flag::read) {
        unix_flag |= O_RDONLY;
    }

    int result = ::open(buffer, unix_flag, unix_mode);
    if (result < 0)
        return std::error_code{errno, std::system_category()};

    if (m_file != -1)
        ::close(m_file);
    m_file = result;

    return std::error_code{};
}

auto coco::binary_file::seek(seek_whence whence,
                             ptrdiff_t   offset) noexcept -> std::error_code {
    int unix_whence =
        (whence == seek_whence::begin
             ? SEEK_SET
             : (whence == seek_whence::current ? SEEK_CUR : SEEK_END));

    auto result = ::lseek(m_file, offset, unix_whence);
    if (result < 0)
        return std::error_code{errno, std::system_category()};

    return std::error_code{};
}

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

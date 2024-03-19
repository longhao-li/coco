#if defined(__LP64__)
#    define _FILE_OFFSET_BITS 64
#endif
#include "coco/file.hpp"

#include <cstring>

using namespace coco;

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

auto coco::binary_file::open(std::string_view path, flag flags) noexcept
    -> std::error_code {
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

namespace {

struct read_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int          m_file;
    unsigned int m_size;
    void        *m_buffer;
    __u64        m_offset;

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

        io_uring_prep_read(sqe, m_file, m_buffer, m_size, m_offset);
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

auto coco::binary_file::read(void *buffer, uint32_t size) noexcept
    -> task<std::error_code> {
    read_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_buffer   = buffer,
        .m_offset   = __u64(-1),
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::binary_file::read(size_t offset, void *buffer,
                             uint32_t size) noexcept -> task<std::error_code> {
    read_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_buffer   = buffer,
        .m_offset   = offset,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::binary_file::read(void *buffer, uint32_t size,
                             uint32_t &bytes) noexcept
    -> task<std::error_code> {
    read_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_buffer   = buffer,
        .m_offset   = __u64(-1),
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    bytes = static_cast<uint32_t>(result);
    co_return std::error_code{};
}

auto coco::binary_file::read(size_t offset, void *buffer, uint32_t size,
                             uint32_t &bytes) noexcept
    -> task<std::error_code> {
    read_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_buffer   = buffer,
        .m_offset   = offset,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    bytes = static_cast<uint32_t>(result);
    co_return std::error_code{};
}

namespace {

struct write_awaitable {
    using promise_type     = promise<std::error_code>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    coco::detail::user_data m_userdata;

    int          m_file;
    unsigned int m_size;
    const void  *m_data;
    __u64        m_offset;

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

        io_uring_prep_write(sqe, m_file, m_data, m_size, m_offset);
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

auto coco::binary_file::write(const void *data, uint32_t size) noexcept
    -> task<std::error_code> {
    write_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_data     = data,
        .m_offset   = __u64(-1),
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::binary_file::write(size_t offset, const void *data,
                              uint32_t size) noexcept -> task<std::error_code> {
    write_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_data     = data,
        .m_offset   = offset,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    co_return std::error_code{};
}

auto coco::binary_file::write(const void *data, uint32_t size,
                              uint32_t &bytes) noexcept
    -> task<std::error_code> {
    write_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_data     = data,
        .m_offset   = __u64(-1),
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    bytes = static_cast<uint32_t>(result);
    co_return std::error_code{};
}

auto coco::binary_file::write(size_t offset, const void *data, uint32_t size,
                              uint32_t &bytes) noexcept
    -> task<std::error_code> {
    write_awaitable awaitable{
        .m_userdata = {},
        .m_file     = m_file,
        .m_size     = size,
        .m_data     = data,
        .m_offset   = offset,
    };

    int result = co_await awaitable;
    if (result < 0)
        co_return std::error_code{-result, std::system_category()};

    bytes = static_cast<uint32_t>(result);
    co_return std::error_code{};
}

auto coco::binary_file::seek(seek_whence whence, ptrdiff_t offset) noexcept
    -> std::error_code {
    int unix_whence =
        (whence == seek_whence::begin
             ? SEEK_SET
             : (whence == seek_whence::current ? SEEK_CUR : SEEK_END));

    auto result = ::lseek(m_file, offset, unix_whence);
    if (result < 0)
        return std::error_code{errno, std::system_category()};

    return std::error_code{};
}

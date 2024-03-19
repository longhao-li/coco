#pragma once

#include "task.hpp"

namespace coco {

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
    COCO_API auto read(void *buffer, uint32_t size) noexcept
        -> task<std::error_code>;

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
    COCO_API auto read(size_t offset, void *buffer, uint32_t size) noexcept
        -> task<std::error_code>;

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
    COCO_API auto read(void *buffer, uint32_t size, uint32_t &bytes) noexcept
        -> task<std::error_code>;

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
    COCO_API auto read(size_t offset, void *buffer, uint32_t size,
                       uint32_t &bytes) noexcept -> task<std::error_code>;

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
    COCO_API auto write(const void *data, uint32_t size) noexcept
        -> task<std::error_code>;

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
    COCO_API auto write(size_t offset, const void *data, uint32_t size) noexcept
        -> task<std::error_code>;

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
    COCO_API auto write(const void *data, uint32_t size,
                        uint32_t &bytes) noexcept -> task<std::error_code>;

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
    COCO_API auto write(size_t offset, const void *data, uint32_t size,
                        uint32_t &bytes) noexcept -> task<std::error_code>;

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

} // namespace coco

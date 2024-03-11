# coco

A lightweight and easy to use async IO library implemented with io_uring and C++20 coroutine.

## Build

This project depends on liburing. Please install `liburing-devel`(SUSE/RHEL based distributions) or `liburing-dev`(Debian based distributions) before building this project.

### CMake Options

|          Option             |                            Description                          | Default Value |
| --------------------------- | --------------------------------------------------------------- | ------------- |
| `COCO_BUILD_STATIC_LIBRARY` | Build coco shared library. The shared target is `coco::shared`. |     `ON`      |
| `COCO_BUILD_SHARED_LIBRARY` | Build coco static library. The static target is `coco::static`. |     `ON`      |
| `COCO_ENABLE_LTO`           | Enable LTO if possible.                                         |     `ON`      |
| `COCO_WARNINGS_AS_ERRORS`   | Treat warnings as errors.                                       |     `OFF`     |
| `COCO_BUILD_EXAMPLES`       | Build coco examples.                                            |     `OFF`     |

If both of the static target and shared target are build, the default target `coco::coco` and `coco` will be set to the static library.

## Examples

### Echo Server

See `examples/echo` for details.

```cpp
auto echo(tcp_connection connection) noexcept -> task<void> {
    char            buffer[4096];
    size_t          bytes;
    std::error_code error;

    for (;;) {
        error = co_await connection.receive(buffer, sizeof(buffer), bytes);
        // Handle errors.
        if (bytes == 0) 
            // Connection closed.

        error = co_await connection.send(buffer, bytes, bytes);
        // Handle errors.
    }
    co_return;
}

auto acceptor(io_context &io_ctx, tcp_server server) noexcept -> task<void> {
    std::error_code error;
    tcp_connection  connection;
    for (;;) {
        error = co_await server.accept(connection);
        // Handle errors.
        io_ctx.execute(echo(std::move(connection)));
    }
}
```

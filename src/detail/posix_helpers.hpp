#pragma once

#include <cpp_core/error_callback.h>
#include <cpp_core/error_handling.hpp>
#include <cpp_core/serial_config.hpp>
#include <cpp_core/status_code.h>
#include <cpp_core/unique_resource.hpp>
#include <cpp_core/validation.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <poll.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <system_error>
#include <unordered_map>
#include <unistd.h>

#include "posix_termios2.hpp"

namespace cpp_bindings_linux::detail
{
using IoCallbackT = void (*)(int);
using StatusCodeValue = cpp_core::StatusCodeValue;
using cpp_core::StatusCode;
using cpp_core::FlowControl;
using cpp_core::Parity;
using cpp_core::StopBits;

enum class Operation
{
    kRead,
    kWrite,
};

enum class WaitResult
{
    kReady,
    kTimedOut,
    kAborted,
    kError,
};

struct PosixFdTraits
{
    using handle_type = int;

    static constexpr auto invalid() noexcept -> handle_type
    {
        return -1;
    }

    static auto close(handle_type fd) noexcept -> void
    {
        if (fd >= 0)
        {
            ::close(fd);
        }
    }
};

using UniqueFd = cpp_core::UniqueResource<PosixFdTraits>;

struct HandleState
{
    std::atomic<int64_t> bytes_read_total{0};
    std::atomic<int64_t> bytes_written_total{0};
    std::atomic<bool> abort_read{false};
    std::atomic<bool> abort_write{false};
};

struct HandleContext
{
    int fd = -1;
    std::shared_ptr<HandleState> state;
};

inline std::mutex g_handle_states_mutex;
inline std::unordered_map<int, std::shared_ptr<HandleState>> g_handle_states;
inline std::atomic<ErrorCallbackT> g_error_callback{nullptr};
inline std::atomic<IoCallbackT> g_read_callback{nullptr};
inline std::atomic<IoCallbackT> g_write_callback{nullptr};

template <typename Code> constexpr auto statusValue(Code code) -> StatusCodeValue
{
    return static_cast<StatusCodeValue>(code);
}

inline auto effectiveErrorCallback(ErrorCallbackT error_callback) -> ErrorCallbackT
{
    return error_callback != nullptr ? error_callback : g_error_callback.load(std::memory_order_acquire);
}

inline auto defaultValidationMessage(StatusCodeValue code) -> std::string_view
{
    switch (code)
    {
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetBaudrateError):
        return "Invalid baudrate: must be >= 300";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetDataBitsError):
        return "Invalid data bits: must be 5-8";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetParityError):
        return "Invalid parity: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetStopBitsError):
        return "Invalid stop bits: must be 0, 1, or 2";
    case static_cast<StatusCodeValue>(StatusCode::Configuration::kSetFlowControlError):
        return "Invalid flow control mode: must be 0, 1, or 2";
    default:
        return "Invalid serial setting";
    }
}

template <typename Ret>
inline auto failMsg(ErrorCallbackT error_callback, StatusCodeValue code, std::string_view message) -> Ret
{
    return cpp_core::failMsg<Ret>(effectiveErrorCallback(error_callback), code, message);
}

template <typename Ret>
inline auto failErrno(ErrorCallbackT error_callback, StatusCodeValue code) -> Ret
{
    return cpp_core::failMsg<Ret>(effectiveErrorCallback(error_callback), code,
                                  std::error_code(errno, std::generic_category()).message());
}

template <typename Ret> inline auto failValidation(ErrorCallbackT error_callback, StatusCodeValue code) -> Ret
{
    return failMsg<Ret>(error_callback, code, defaultValidationMessage(code));
}

inline auto ensureHandleState(int fd) -> std::shared_ptr<HandleState>
{
    std::lock_guard lock(g_handle_states_mutex);
    auto &state = g_handle_states[fd];
    if (!state)
    {
        state = std::make_shared<HandleState>();
    }
    return state;
}

inline auto removeHandleState(int fd) -> void
{
    std::lock_guard lock(g_handle_states_mutex);
    g_handle_states.erase(fd);
}

template <typename Ret>
inline auto validatePosixFd(int64_t handle, ErrorCallbackT error_callback, int *out_fd) -> Ret
{
    const auto rc = cpp_core::validateHandle<Ret>(handle, effectiveErrorCallback(error_callback));
    if (rc < 0)
    {
        return rc;
    }

    *out_fd = static_cast<int>(handle);
    return static_cast<Ret>(StatusCode::kSuccess);
}

template <typename Ret>
inline auto acquireHandleContext(int64_t handle, ErrorCallbackT error_callback, HandleContext *out_context) -> Ret
{
    int fd = -1;
    const auto rc = validatePosixFd<Ret>(handle, error_callback, &fd);
    if (rc < 0)
    {
        return rc;
    }

    out_context->fd = fd;
    out_context->state = ensureHandleState(fd);
    return static_cast<Ret>(StatusCode::kSuccess);
}

inline auto registerOpenedHandle(int fd) -> void
{
    (void)ensureHandleState(fd);
}

inline auto setAbortFlag(const std::shared_ptr<HandleState> &state, Operation operation) -> void
{
    if (operation == Operation::kRead)
    {
        state->abort_read.store(true, std::memory_order_release);
    }
    else
    {
        state->abort_write.store(true, std::memory_order_release);
    }
}

inline auto consumeAbortFlag(const std::shared_ptr<HandleState> &state, Operation operation) -> bool
{
    if (operation == Operation::kRead)
    {
        return state->abort_read.exchange(false, std::memory_order_acq_rel);
    }

    return state->abort_write.exchange(false, std::memory_order_acq_rel);
}

inline auto invokeIoCallback(IoCallbackT callback, int bytes) -> void
{
    if (callback != nullptr)
    {
        callback(bytes);
    }
}

inline auto noteBytesRead(const std::shared_ptr<HandleState> &state, int bytes_read) -> void
{
    state->bytes_read_total.fetch_add(bytes_read, std::memory_order_relaxed);
    invokeIoCallback(g_read_callback.load(std::memory_order_acquire), bytes_read);
}

inline auto noteBytesWritten(const std::shared_ptr<HandleState> &state, int bytes_written) -> void
{
    state->bytes_written_total.fetch_add(bytes_written, std::memory_order_relaxed);
    invokeIoCallback(g_write_callback.load(std::memory_order_acquire), bytes_written);
}

inline auto bytesReadTotal(const std::shared_ptr<HandleState> &state) -> int64_t
{
    return state->bytes_read_total.load(std::memory_order_relaxed);
}

inline auto bytesWrittenTotal(const std::shared_ptr<HandleState> &state) -> int64_t
{
    return state->bytes_written_total.load(std::memory_order_relaxed);
}

inline auto waitFdReady(const std::shared_ptr<HandleState> &state, int file_descriptor, int timeout_ms,
                        Operation operation) -> WaitResult
{
    if (consumeAbortFlag(state, operation))
    {
        return WaitResult::kAborted;
    }

    const short requested_events = operation == Operation::kRead ? POLLIN : POLLOUT;
    int remaining_timeout_ms = std::max(timeout_ms, 0);

    while (true)
    {
        pollfd poll_fd{
            .fd = file_descriptor,
            .events = requested_events,
            .revents = 0,
        };

        const int slice_timeout_ms = timeout_ms == 0 ? 0 : std::min(remaining_timeout_ms, 50);
        const int poll_result = poll(&poll_fd, 1, slice_timeout_ms);
        if (poll_result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return WaitResult::kError;
        }

        if (poll_result > 0)
        {
            if ((poll_fd.revents & requested_events) != 0 || (poll_fd.revents & (POLLERR | POLLHUP)) != 0)
            {
                return WaitResult::kReady;
            }
        }

        if (consumeAbortFlag(state, operation))
        {
            return WaitResult::kAborted;
        }

        if (timeout_ms == 0)
        {
            return WaitResult::kTimedOut;
        }

        remaining_timeout_ms -= slice_timeout_ms;
        if (remaining_timeout_ms <= 0)
        {
            return WaitResult::kTimedOut;
        }
    }
}

inline auto multiplierTimeout(int timeout_ms, int multiplier) -> int
{
    if (multiplier <= 0)
    {
        return 0;
    }

    return cpp_core::clampTimeout(timeout_ms) * multiplier;
}

inline auto validateBaudrateValue(int baudrate) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(baudrate, 8))
    {
        return cpp_core::ok();
    }
    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetBaudrateError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetBaudrateError))));
}

inline auto validateDataBitsValue(int data_bits) -> cpp_core::Status
{
    if (cpp_core::SerialConfig::tryMake(300, data_bits))
    {
        return cpp_core::ok();
    }
    return cpp_core::fail<>(statusValue(StatusCode::Configuration::kSetDataBitsError),
                            std::string(defaultValidationMessage(
                                statusValue(StatusCode::Configuration::kSetDataBitsError))));
}

inline auto parseParity(int parity, ErrorCallbackT error_callback, StatusCodeValue invalid_code) -> std::optional<Parity>
{
    switch (parity)
    {
    case 0:
        return Parity::kNone;
    case 1:
        return Parity::kEven;
    case 2:
        return Parity::kOdd;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}

inline auto parseStopBits(int stop_bits, ErrorCallbackT error_callback, StatusCodeValue invalid_code,
                          bool allow_one_alias = true) -> std::optional<StopBits>
{
    switch (stop_bits)
    {
    case 0:
        return StopBits::kOne;
    case 1:
        if (allow_one_alias)
        {
            return StopBits::kOne;
        }
        break;
    case 2:
        return StopBits::kTwo;
    default:
        break;
    }

    (void)failValidation<int>(error_callback, invalid_code);
    return std::nullopt;
}

inline auto parseFlowControl(int mode, ErrorCallbackT error_callback, StatusCodeValue invalid_code)
    -> std::optional<FlowControl>
{
    switch (mode)
    {
    case 0:
        return FlowControl::kNone;
    case 1:
        return FlowControl::kRtsCts;
    case 2:
        return FlowControl::kXonXoff;
    default:
        (void)failValidation<int>(error_callback, invalid_code);
        return std::nullopt;
    }
}

template <typename Ret>
inline auto readTermios2(int fd, termios2 *tty, ErrorCallbackT error_callback) -> Ret
{
    if (ioctl(fd, TCGETS2, tty) != 0)
    {
        return failErrno<Ret>(error_callback, statusValue(StatusCode::Control::kGetStateError));
    }
    return static_cast<Ret>(StatusCode::kSuccess);
}

template <typename Ret>
inline auto writeTermios2(int fd, termios2 *tty, ErrorCallbackT error_callback, StatusCodeValue set_error_code) -> Ret
{
    if (ioctl(fd, TCSETS2, tty) != 0)
    {
        return failErrno<Ret>(error_callback, set_error_code);
    }
    return static_cast<Ret>(StatusCode::kSuccess);
}

inline auto applyBaudrate(termios2 *tty, int baudrate) -> void
{
    tty->c_cflag &= ~CBAUD;
    tty->c_cflag |= BOTHER;
    tty->c_ispeed = static_cast<speed_t>(baudrate);
    tty->c_ospeed = static_cast<speed_t>(baudrate);
}

inline auto applyDataBits(termios2 *tty, int data_bits) -> void
{
    tty->c_cflag &= ~CSIZE;
    switch (data_bits)
    {
    case 5:
        tty->c_cflag |= CS5;
        break;
    case 6:
        tty->c_cflag |= CS6;
        break;
    case 7:
        tty->c_cflag |= CS7;
        break;
    case 8:
    default:
        tty->c_cflag |= CS8;
        break;
    }
}

inline auto applyParity(termios2 *tty, Parity parity) -> void
{
    tty->c_cflag &= ~(PARENB | PARODD);
    if (parity == Parity::kEven)
    {
        tty->c_cflag |= PARENB;
    }
    else if (parity == Parity::kOdd)
    {
        tty->c_cflag |= (PARENB | PARODD);
    }
}

inline auto applyStopBits(termios2 *tty, StopBits stop_bits) -> void
{
    if (stop_bits == StopBits::kTwo)
    {
        tty->c_cflag |= CSTOPB;
    }
    else
    {
        tty->c_cflag &= ~CSTOPB;
    }
}

inline auto applyFlowControl(termios2 *tty, FlowControl flow_control) -> void
{
    tty->c_cflag &= ~CRTSCTS;
    tty->c_iflag &= ~(IXON | IXOFF | IXANY);

    if (flow_control == FlowControl::kRtsCts)
    {
        tty->c_cflag |= CRTSCTS;
    }
    else if (flow_control == FlowControl::kXonXoff)
    {
        tty->c_iflag |= (IXON | IXOFF);
    }
}

inline auto trimWhitespace(std::string value) -> std::string
{
    const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
    {
        value.pop_back();
    }
    return value;
}

inline auto readTrimmedFile(const std::filesystem::path &path) -> std::optional<std::string>
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        return std::nullopt;
    }

    std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    contents = trimWhitespace(std::move(contents));
    if (contents.empty())
    {
        return std::nullopt;
    }
    return contents;
}

inline auto isSerialDeviceName(std::string_view name) -> bool
{
    static constexpr std::string_view kPrefixes[] = {"ttyUSB", "ttyACM", "ttyS", "ttyAMA", "rfcomm", "ttyTHS"};
    return std::ranges::any_of(kPrefixes, [name](std::string_view prefix) { return name.starts_with(prefix); });
}

inline auto matchesSuffix(const unsigned char *buffer, int buffer_size, const unsigned char *terminator,
                          int terminator_size) -> bool
{
    if (terminator == nullptr || terminator_size <= 0 || buffer_size < terminator_size)
    {
        return false;
    }

    return std::memcmp(buffer + (buffer_size - terminator_size), terminator,
                       static_cast<std::size_t>(terminator_size)) == 0;
}

inline auto readImpl(int64_t handle, void *buffer, int buffer_size, int timeout_ms, int multiplier,
                     const unsigned char *terminator, int terminator_size, ErrorCallbackT error_callback) -> int
{
    const auto callback = effectiveErrorCallback(error_callback);
    const auto buffer_rc = cpp_core::validateBuffer<int>(buffer, buffer_size, callback);
    if (buffer_rc < 0)
    {
        return buffer_rc;
    }

    if (terminator_size > 0 && terminator == nullptr)
    {
        return failMsg<int>(error_callback, statusValue(StatusCode::Io::kBufferError), "Invalid terminator");
    }

    HandleContext context;
    const auto handle_rc = acquireHandleContext<int>(handle, callback, &context);
    if (handle_rc < 0)
    {
        return handle_rc;
    }

    auto *output = static_cast<unsigned char *>(buffer);
    const bool read_single_bytes = terminator_size > 0;
    int total_read = 0;

    while (total_read < buffer_size)
    {
        const int current_timeout_ms =
            total_read == 0 ? cpp_core::clampTimeout(timeout_ms) : multiplierTimeout(timeout_ms, multiplier);
        switch (waitFdReady(context.state, context.fd, current_timeout_ms, Operation::kRead))
        {
        case WaitResult::kTimedOut:
            return total_read;
        case WaitResult::kAborted:
            return failMsg<int>(error_callback, statusValue(StatusCode::Io::kAbortReadError), "Read aborted");
        case WaitResult::kError:
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kReadError));
        case WaitResult::kReady:
            break;
        }

        const int chunk_size = read_single_bytes ? 1 : (buffer_size - total_read);
        const ssize_t bytes_read = ::read(context.fd, output + total_read, static_cast<std::size_t>(chunk_size));
        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kReadError));
        }

        if (bytes_read == 0)
        {
            return total_read;
        }

        noteBytesRead(context.state, static_cast<int>(bytes_read));
        total_read += static_cast<int>(bytes_read);

        if (matchesSuffix(output, total_read, terminator, terminator_size))
        {
            return total_read;
        }
    }

    return total_read;
}

inline auto writeImpl(int64_t handle, const void *buffer, int buffer_size, int timeout_ms, int multiplier,
                      ErrorCallbackT error_callback) -> int
{
    const auto callback = effectiveErrorCallback(error_callback);
    const auto buffer_rc = cpp_core::validateBuffer<int>(buffer, buffer_size, callback);
    if (buffer_rc < 0)
    {
        return buffer_rc;
    }

    HandleContext context;
    const auto handle_rc = acquireHandleContext<int>(handle, callback, &context);
    if (handle_rc < 0)
    {
        return handle_rc;
    }

    const auto *input = static_cast<const unsigned char *>(buffer);
    int total_written = 0;

    while (total_written < buffer_size)
    {
        const int current_timeout_ms =
            total_written == 0 ? cpp_core::clampTimeout(timeout_ms) : multiplierTimeout(timeout_ms, multiplier);
        switch (waitFdReady(context.state, context.fd, current_timeout_ms, Operation::kWrite))
        {
        case WaitResult::kTimedOut:
            return total_written;
        case WaitResult::kAborted:
            return failMsg<int>(error_callback, statusValue(StatusCode::Io::kAbortWriteError), "Write aborted");
        case WaitResult::kError:
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kWriteError));
        case WaitResult::kReady:
            break;
        }

        const ssize_t bytes_written =
            ::write(context.fd, input + total_written, static_cast<std::size_t>(buffer_size - total_written));
        if (bytes_written < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            return failErrno<int>(error_callback, statusValue(StatusCode::Io::kWriteError));
        }

        if (bytes_written == 0)
        {
            return total_written;
        }

        noteBytesWritten(context.state, static_cast<int>(bytes_written));
        total_written += static_cast<int>(bytes_written);
    }

    return total_written;
}

} // namespace cpp_bindings_linux::detail

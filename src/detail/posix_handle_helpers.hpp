#pragma once

#include "posix_common.hpp"

#include <cpp_core/unique_resource.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unistd.h>

namespace cpp_bindings_linux::detail
{
enum class Operation
{
    kRead,
    kWrite,
};

struct PosixFdTraits
{
    using handle_type = int;

    static constexpr auto invalid() noexcept -> handle_type
    {
        return -1;
    }

    static auto close(handle_type file_descriptor) noexcept -> void
    {
        if (file_descriptor >= 0)
        {
            ::close(file_descriptor);
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
    int file_descriptor = -1;
    std::shared_ptr<HandleState> state;
};

inline std::mutex g_handle_states_mutex;
inline std::unordered_map<int, std::shared_ptr<HandleState>> g_handle_states;
inline std::atomic<IoCallbackT> g_read_callback{nullptr};
inline std::atomic<IoCallbackT> g_write_callback{nullptr};

inline auto ensureHandleState(int file_descriptor) -> std::shared_ptr<HandleState>
{
    std::lock_guard lock(g_handle_states_mutex);
    auto &handle_state = g_handle_states[file_descriptor];
    if (!handle_state)
    {
        handle_state = std::make_shared<HandleState>();
    }
    return handle_state;
}

inline auto removeHandleState(int file_descriptor) -> void
{
    std::lock_guard lock(g_handle_states_mutex);
    g_handle_states.erase(file_descriptor);
}

template <typename Ret>
inline auto validatePosixFileDescriptor(int64_t handle, ErrorCallbackT error_callback, int *out_file_descriptor) -> Ret
{
    const auto validation_status = cpp_core::validateHandle<Ret>(handle, effectiveErrorCallback(error_callback));
    if (validation_status < 0)
    {
        return validation_status;
    }

    *out_file_descriptor = static_cast<int>(handle);
    return static_cast<Ret>(StatusCode::kSuccess);
}

template <typename Ret>
inline auto acquireHandleContext(int64_t handle, ErrorCallbackT error_callback, HandleContext *out_handle_context)
    -> Ret
{
    int file_descriptor = -1;
    const auto validation_status =
        validatePosixFileDescriptor<Ret>(handle, error_callback, &file_descriptor);
    if (validation_status < 0)
    {
        return validation_status;
    }

    out_handle_context->file_descriptor = file_descriptor;
    out_handle_context->state = ensureHandleState(file_descriptor);
    return static_cast<Ret>(StatusCode::kSuccess);
}

inline auto registerOpenedHandle(int file_descriptor) -> void
{
    (void)ensureHandleState(file_descriptor);
}

inline auto setAbortFlag(const std::shared_ptr<HandleState> &handle_state, Operation operation) -> void
{
    if (operation == Operation::kRead)
    {
        handle_state->abort_read.store(true, std::memory_order_release);
    }
    else
    {
        handle_state->abort_write.store(true, std::memory_order_release);
    }
}

inline auto consumeAbortFlag(const std::shared_ptr<HandleState> &handle_state, Operation operation) -> bool
{
    if (operation == Operation::kRead)
    {
        return handle_state->abort_read.exchange(false, std::memory_order_acq_rel);
    }

    return handle_state->abort_write.exchange(false, std::memory_order_acq_rel);
}

inline auto invokeIoCallback(IoCallbackT callback, int transferred_bytes) -> void
{
    if (callback != nullptr)
    {
        callback(transferred_bytes);
    }
}

inline auto noteBytesRead(const std::shared_ptr<HandleState> &handle_state, int bytes_read) -> void
{
    handle_state->bytes_read_total.fetch_add(bytes_read, std::memory_order_relaxed);
    invokeIoCallback(g_read_callback.load(std::memory_order_acquire), bytes_read);
}

inline auto noteBytesWritten(const std::shared_ptr<HandleState> &handle_state, int bytes_written) -> void
{
    handle_state->bytes_written_total.fetch_add(bytes_written, std::memory_order_relaxed);
    invokeIoCallback(g_write_callback.load(std::memory_order_acquire), bytes_written);
}

inline auto bytesReadTotal(const std::shared_ptr<HandleState> &handle_state) -> int64_t
{
    return handle_state->bytes_read_total.load(std::memory_order_relaxed);
}

inline auto bytesWrittenTotal(const std::shared_ptr<HandleState> &handle_state) -> int64_t
{
    return handle_state->bytes_written_total.load(std::memory_order_relaxed);
}

} // namespace cpp_bindings_linux::detail

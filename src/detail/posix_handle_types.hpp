#pragma once

#include "posix_common_types.hpp"

#include <cpp_core/unique_resource.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <unordered_map>

namespace cpp_bindings_linux::detail
{
enum class Operation
{
    kRead,
    kWrite,
};

struct PosixFdTraits
{
    using handle_type = int; // NOLINT(readability-identifier-naming)

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
} // namespace cpp_bindings_linux::detail

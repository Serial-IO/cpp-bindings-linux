#include <cpp_core/interface/serial_list_ports.h>

#include "detail/posix_fail_msg.hpp"
#include "detail/posix_is_serial_device_name.hpp"
#include "detail/posix_read_trimmed_file.hpp"
#include "detail/posix_status_value.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

struct PortInfo
{
    std::string port;
    std::string path;
    std::string manufacturer;
    std::string serial_number;
    std::string pnp_id;
    std::string location_id;
    std::string product_id;
    std::string vendor_id;
};

auto maybeReadAncestorFile(const std::filesystem::path &base_path, std::string_view filename) -> std::optional<std::string>
{
    std::filesystem::path current = base_path;
    for (int depth = 0; depth < 5; ++depth)
    {
        if (const auto value = cpp_bindings_linux::detail::readTrimmedFile(current / filename))
        {
            return value;
        }

        if (!current.has_parent_path())
        {
            break;
        }
        current = current.parent_path();
    }

    return std::nullopt;
}

auto optionalCString(const std::string &value) -> const char *
{
    return value.empty() ? nullptr : value.c_str();
}

} // namespace

extern "C"
{

    MODULE_API auto serialListPorts(void (*callback_fn)(const char *port, const char *path, const char *manufacturer,
                                                        const char *serial_number, const char *pnp_id,
                                                        const char *location_id, const char *product_id,
                                                        const char *vendor_id),
                                    ErrorCallbackT error_callback) -> int
    {
        if (callback_fn == nullptr)
        {
            return cpp_bindings_linux::detail::failMsg<int>(
                error_callback, cpp_bindings_linux::detail::statusValue(cpp_core::StatusCode::Io::kBufferError),
                "Port callback must not be null");
        }

        const std::filesystem::path sysfs_root{"/sys/class/tty"};
        if (!std::filesystem::exists(sysfs_root))
        {
            return 0;
        }

        std::vector<std::filesystem::path> tty_entries;
        for (const auto &entry : std::filesystem::directory_iterator(sysfs_root))
        {
            const auto name = entry.path().filename().string();
            if (cpp_bindings_linux::detail::isSerialDeviceName(name))
            {
                tty_entries.push_back(entry.path());
            }
        }

        std::ranges::sort(tty_entries);

        int count = 0;
        for (const auto &entry : tty_entries)
        {
            PortInfo info;
            info.port = entry.filename().string();
            info.path = "/dev/" + info.port;

            std::error_code error_code;
            auto device_path = std::filesystem::weakly_canonical(entry / "device", error_code);
            if (error_code)
            {
                device_path = entry / "device";
            }

            if (const auto value = maybeReadAncestorFile(device_path, "manufacturer"))
            {
                info.manufacturer = *value;
            }
            if (const auto value = maybeReadAncestorFile(device_path, "serial"))
            {
                info.serial_number = *value;
            }
            if (const auto value = maybeReadAncestorFile(device_path, "modalias"))
            {
                info.pnp_id = *value;
            }
            if (const auto value = maybeReadAncestorFile(device_path, "devpath"))
            {
                info.location_id = *value;
            }
            if (const auto value = maybeReadAncestorFile(device_path, "idProduct"))
            {
                info.product_id = *value;
            }
            if (const auto value = maybeReadAncestorFile(device_path, "idVendor"))
            {
                info.vendor_id = *value;
            }

            callback_fn(optionalCString(info.port), optionalCString(info.path), optionalCString(info.manufacturer),
                        optionalCString(info.serial_number), optionalCString(info.pnp_id), optionalCString(info.location_id),
                        optionalCString(info.product_id), optionalCString(info.vendor_id));
            ++count;
        }

        return count;
    }

} // extern "C"

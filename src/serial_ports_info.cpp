#include "serial.h"
#include "status_codes.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace
{
namespace fs = std::filesystem;

inline void invokeError(int code, const char* message)
{
    if (on_error_callback != nullptr)
    {
        on_error_callback(code, message);
    }
}

// Reads a single attribute file from a given directory. Returns empty string on error.
std::string readAttr(const fs::path& dir, const std::string& attr)
{
    std::ifstream file(dir / attr);
    if (!file.is_open())
    {
        return "";
    }
    std::string value;
    std::getline(file, value);
    return value;
}

struct UsbInfo
{
    std::string manufacturer;
    std::string serial_number;
    std::string vendor_id;
    std::string product_id;
    std::string pnp_id;
    std::string location_id;
};

// Attempts to locate the USB device directory for a tty canonical path and collect attributes.
UsbInfo collectUsbInfo(const fs::path& canonicalPath)
{
    UsbInfo info{};

    std::error_code error_code;
    fs::path tty_sys = fs::path{"/sys/class/tty"} / canonicalPath.filename() / "device";
    tty_sys = fs::canonical(tty_sys, error_code);
    if (error_code)
    {
        return info; // return empty info if we cannot resolve
    }

    fs::path usb_dir;
    fs::path cur = tty_sys;
    while (cur.has_relative_path() && cur != cur.root_path())
    {
        if (fs::exists(cur / "idVendor"))
        {
            usb_dir = cur;
            break;
        }
        cur = cur.parent_path();
    }

    if (usb_dir.empty())
    {
        return info; // not a USB device (e.g., built-in tty)
    }

    info.manufacturer = readAttr(usb_dir, "manufacturer");
    info.serial_number = readAttr(usb_dir, "serial");
    info.vendor_id = readAttr(usb_dir, "idVendor");
    info.product_id = readAttr(usb_dir, "idProduct");

    if (!info.vendor_id.empty() && !info.product_id.empty())
    {
        info.pnp_id = "USB\\VID_" + info.vendor_id + "&PID_" + info.product_id;
    }

    const std::string busnum = readAttr(usb_dir, "busnum");
    const std::string devpath = readAttr(usb_dir, "devpath");
    if (!busnum.empty() && !devpath.empty())
    {
        info.location_id = busnum + ":" + devpath;
    }

    return info;
}

// Handles a single directory entry. Returns true if a port was reported.
bool handleEntry(const fs::directory_entry& entry,
                 void (*callback)(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*))
{
    if (!entry.is_symlink())
    {
        return false;
    }

    std::error_code error_code;
    fs::path canonical = fs::canonical(entry.path(), error_code);
    if (error_code)
    {
        return false;
    }

    const std::string port_path = canonical.string();
    const std::string symlink_path = entry.path().string();

    UsbInfo info = collectUsbInfo(canonical);

    if (callback != nullptr)
    {
        callback(port_path.c_str(),
                 symlink_path.c_str(),
                 info.manufacturer.c_str(),
                 info.serial_number.c_str(),
                 info.pnp_id.c_str(),
                 info.location_id.c_str(),
                 info.product_id.c_str(),
                 info.vendor_id.c_str());
    }
    return true;
}

} // namespace

int serialGetPortsInfo(void (*callback)(const char* port,
                                        const char* path,
                                        const char* manufacturer,
                                        const char* serialNumber,
                                        const char* pnpId,
                                        const char* locationId,
                                        const char* productId,
                                        const char* vendorId))
{
    const fs::path by_id_dir{"/dev/serial/by-id"};
    if (!fs::exists(by_id_dir) || !fs::is_directory(by_id_dir))
    {
        invokeError(std::to_underlying(StatusCodes::NOT_FOUND_ERROR), "serialGetPortsInfo: Failed to get ports info");
        return 0;
    }

    int count = 0;

    try
    {
        for (const auto& entry : fs::directory_iterator{by_id_dir})
        {
            if (handleEntry(entry, callback))
            {
                ++count;
            }
        }
    }
    catch (const fs::filesystem_error&)
    {
        invokeError(std::to_underlying(StatusCodes::NOT_FOUND_ERROR), "serialGetPortsInfo: Failed to get ports info");
        return 0;
    }

    return count;
}

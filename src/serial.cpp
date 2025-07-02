#include "serial.h"
#include "status_codes.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

// -----------------------------------------------------------------------------
// Internal helpers & types
// -----------------------------------------------------------------------------
namespace {

struct SerialPortHandle {
  int fd;
  termios original; // keep original settings so we can restore on close
};

// Map integer baudrate to POSIX speed_t. Only common rates are supported.
static auto to_speed_t(int baud) -> speed_t {
  switch (baud) {
  case 0:
    return B0;
  case 50:
    return B50;
  case 75:
    return B75;
  case 110:
    return B110;
  case 134:
    return B134;
  case 150:
    return B150;
  case 200:
    return B200;
  case 300:
    return B300;
  case 600:
    return B600;
  case 1200:
    return B1200;
  case 1800:
    return B1800;
  case 2400:
    return B2400;
  case 4800:
    return B4800;
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
#ifdef B460800
  case 460800:
    return B460800;
#endif
#ifdef B921600
  case 921600:
    return B921600;
#endif
  default:
    return B9600; // fallback
  }
}

inline void invokeError(int code) {
  if (errorCallback) {
    errorCallback(code);
  }
}

} // namespace

// -----------------------------------------------------------------------------
// Public API implementation
// -----------------------------------------------------------------------------

intptr_t serialOpen(void *port, int baudrate, int dataBits, int parity,
                    int stopBits) {
  if (!port) {
    invokeError(status(StatusCodes::INVALID_HANDLE_ERROR));
    return 0;
  }

  const char *portName = static_cast<const char *>(port);
  int fd = open(portName, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    invokeError(status(StatusCodes::INVALID_HANDLE_ERROR));
    return 0;
  }

  SerialPortHandle *handle = new SerialPortHandle{fd, {}};

  termios tty{};
  if (tcgetattr(fd, &tty) != 0) {
    invokeError(status(StatusCodes::GET_STATE_ERROR));
    close(fd);
    delete handle;
    return 0;
  }
  handle->original = tty; // save original

  // Basic flags: local connection, enable receiver
  tty.c_cflag |= (CLOCAL | CREAD);

  // Baudrate
  const speed_t speed = to_speed_t(baudrate);
  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  // Data bits
  tty.c_cflag &= ~CSIZE;
  switch (dataBits) {
  case 5:
    tty.c_cflag |= CS5;
    break;
  case 6:
    tty.c_cflag |= CS6;
    break;
  case 7:
    tty.c_cflag |= CS7;
    break;
  default:
    tty.c_cflag |= CS8;
    break;
  }

  // Parity
  if (parity == 0) {
    tty.c_cflag &= ~PARENB;
  } else {
    tty.c_cflag |= PARENB;
    if (parity == 1) {
      tty.c_cflag &= ~PARODD; // even
    } else {
      tty.c_cflag |= PARODD; // odd
    }
  }

  // Stop bits
  if (stopBits == 2) {
    tty.c_cflag |= CSTOPB;
  } else {
    tty.c_cflag &= ~CSTOPB;
  }

  // Raw mode (no echo/processing)
  tty.c_iflag = 0;
  tty.c_oflag = 0;
  tty.c_lflag = 0;

  tty.c_cc[VMIN] = 0;   // non-blocking by default
  tty.c_cc[VTIME] = 10; // 1s read timeout

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    invokeError(status(StatusCodes::SET_STATE_ERROR));
    close(fd);
    delete handle;
    return 0;
  }

  return reinterpret_cast<intptr_t>(handle);
}

void serialClose(int64_t handlePtr) {
  auto *handle = reinterpret_cast<SerialPortHandle *>(handlePtr);
  if (!handle)
    return;

  tcsetattr(handle->fd, TCSANOW, &handle->original); // restore
  if (close(handle->fd) != 0) {
    invokeError(status(StatusCodes::CLOSE_HANDLE_ERROR));
  }
  delete handle;
}

static int waitFdReady(int fd, int timeoutMs, bool wantWrite) {
  if (timeoutMs < 0)
    timeoutMs = 0;

  fd_set set;
  FD_ZERO(&set);
  FD_SET(fd, &set);

  timeval tv{};
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;

  int res = select(fd + 1, wantWrite ? nullptr : &set,
                   wantWrite ? &set : nullptr, nullptr, &tv);
  return res; // 0 timeout, -1 error, >0 ready
}

int serialRead(int64_t handlePtr, void *buffer, int bufferSize, int timeout,
               int /*multiplier*/) {
  auto *handle = reinterpret_cast<SerialPortHandle *>(handlePtr);
  if (!handle) {
    invokeError(status(StatusCodes::INVALID_HANDLE_ERROR));
    return 0;
  }

  if (waitFdReady(handle->fd, timeout, false) <= 0) {
    return 0; // timeout or error (we ignore error for now)
  }

  ssize_t n = read(handle->fd, buffer, bufferSize);
  if (n < 0) {
    invokeError(status(StatusCodes::READ_ERROR));
    return 0;
  }
  if (readCallback)
    readCallback(static_cast<int>(n));
  return static_cast<int>(n);
}

int serialWrite(int64_t handlePtr, const void *buffer, int bufferSize,
                int timeout, int /*multiplier*/) {
  auto *handle = reinterpret_cast<SerialPortHandle *>(handlePtr);
  if (!handle) {
    invokeError(status(StatusCodes::INVALID_HANDLE_ERROR));
    return 0;
  }

  if (waitFdReady(handle->fd, timeout, true) <= 0) {
    return 0; // timeout or error
  }

  ssize_t n = write(handle->fd, buffer, bufferSize);
  if (n < 0) {
    invokeError(status(StatusCodes::WRITE_ERROR));
    return 0;
  }
  if (writeCallback)
    writeCallback(static_cast<int>(n));
  return static_cast<int>(n);
}

int serialReadUntil(int64_t handlePtr, void *buffer, int bufferSize,
                    int timeout, int /*multiplier*/, void *untilCharPtr) {
  auto *handle = reinterpret_cast<SerialPortHandle *>(handlePtr);
  if (!handle) {
    invokeError(status(StatusCodes::INVALID_HANDLE_ERROR));
    return 0;
  }

  char untilChar = *static_cast<char *>(untilCharPtr);
  int total = 0;
  auto *buf = static_cast<char *>(buffer);

  while (total < bufferSize) {
    int res = serialRead(handlePtr, buf + total, 1, timeout, 1);
    if (res <= 0)
      break; // timeout or error
    if (buf[total] == untilChar) {
      total += 1;
      break;
    }
    total += res;
  }

  if (readCallback)
    readCallback(total);
  return total;
}

int serialGetPortsInfo(void *buffer, int bufferSize, void *separatorPtr) {
  const char *sep = static_cast<const char *>(separatorPtr);
  std::string result;

  DIR *dir = opendir("/dev/serial/by-id");
  if (!dir) {
    invokeError(status(StatusCodes::NOT_FOUND_ERROR));
    return 0;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_LNK) {
      std::string symlinkPath =
          std::string("/dev/serial/by-id/") + entry->d_name;
      char canonicalPath[PATH_MAX];
      if (realpath(symlinkPath.c_str(), canonicalPath)) {
        result += canonicalPath;
        result += sep;
      }
    }
  }
  closedir(dir);

  if (!result.empty()) {
    result.erase(result.size() - std::strlen(sep)); // remove trailing sep
  }

  if (static_cast<int>(result.size()) + 1 > bufferSize) {
    invokeError(status(StatusCodes::BUFFER_ERROR));
    return 0;
  }

  std::memcpy(buffer, result.c_str(), result.size() + 1);
  return static_cast<int>(
      result.empty() ? 0 : 1); // number of ports not easily counted here
}

// Currently stubbed helpers (no-ops)
void serialClearBufferIn(int64_t) {}
void serialClearBufferOut(int64_t) {}
void serialAbortRead(int64_t) {}
void serialAbortWrite(int64_t) {}

// Callback registration
void serialOnError(void (*func)(int code)) { errorCallback = func; }
void serialOnRead(void (*func)(int bytes)) { readCallback = func; }
void serialOnWrite(void (*func)(int bytes)) { writeCallback = func; }

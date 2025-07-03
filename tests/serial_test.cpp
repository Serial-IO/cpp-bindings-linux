// Simple integration test for CPP-Unix-Bindings
// ------------------------------------------------
// This executable opens the given serial port, sends a test
// string and verifies that the same string is echoed back
// by the micro-controller.
//
// ------------------------------------------------
// Arduino sketch to flash for the tests
// ------------------------------------------------
/*
  // --- BEGIN ARDUINO CODE ---
  void setup() {
      Serial.begin(115200);
      // Wait until the host opens the port (optional but handy)
      while (!Serial) {
          ;
      }
  }

  void loop() {
      if (Serial.available()) {
          char c = Serial.read();
          Serial.write(c);          // echo back
      }
  }
  // --- END ARDUINO CODE ---
*/
// ------------------------------------------------

#include "serial.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

int main(int argc, char* argv[])
{
    const char* port = argc > 1 ? argv[1] : "/dev/ttyUSB0"; // default path
    const std::string test_msg = "HELLO";

    intptr_t handle = serialOpen((void*)port, 115200, 8, 0, 0);
    if (handle == 0)
    {
        std::cerr << "Failed to open port " << port << "\n";
        return 1;
    }

    // Opening a serial connection toggles DTR on most Arduino boards, which
    // triggers a reset.  Give the micro-controller a moment to reboot before we
    // start talking to it, otherwise the first bytes might be lost.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Send message
    int written = serialWrite(handle, (void*)test_msg.c_str(), test_msg.size(), 100, 1);
    if (std::cmp_not_equal(written, test_msg.size()))
    {
        std::cerr << "Write failed\n";
        serialClose(handle);
        return 2;
    }

    // Read echo
    char buffer[16] = {0};
    int read_bytes = serialRead(handle, buffer, test_msg.size(), 500, 1);
    if (std::cmp_not_equal(read_bytes, test_msg.size()))
    {
        std::cerr << "Read failed (got " << read_bytes << ")\n";
        serialClose(handle);
        return 3;
    }

    if (std::strncmp(buffer, test_msg.c_str(), test_msg.size()) != 0)
    {
        std::cerr << "Data mismatch: expected " << test_msg << ", got " << buffer << "\n";
        serialClose(handle);
        return 4;
    }

    std::cout << "Serial echo test passed on port " << port << "\n";
    serialClose(handle);
    return 0;
}

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
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  const char *port = argc > 1 ? argv[1] : "/dev/ttyACM0"; // default path
  const std::string testMsg = "HELLO";

  intptr_t handle = serialOpen((void *)port, 115200, 8, 0, 0);
  if (!handle) {
    std::cerr << "Failed to open port " << port << "\n";
    return 1;
  }

  // Send message
  int written =
      serialWrite(handle, (void *)testMsg.c_str(), testMsg.size(), 100, 1);
  if (written != static_cast<int>(testMsg.size())) {
    std::cerr << "Write failed\n";
    serialClose(handle);
    return 2;
  }

  // Read echo
  char buffer[16] = {0};
  int readBytes = serialRead(handle, buffer, testMsg.size(), 500, 1);
  if (readBytes != static_cast<int>(testMsg.size())) {
    std::cerr << "Read failed (got " << readBytes << ")\n";
    serialClose(handle);
    return 3;
  }

  if (std::strncmp(buffer, testMsg.c_str(), testMsg.size()) != 0) {
    std::cerr << "Data mismatch: expected " << testMsg << ", got " << buffer
              << "\n";
    serialClose(handle);
    return 4;
  }

  std::cout << "Serial echo test passed on port " << port << "\n";
  serialClose(handle);
  return 0;
}

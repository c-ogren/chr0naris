#include "Scoreboard.hpp"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <ratio>
#include <thread>

const uint8_t SPACE_ASCII = 0x20;
const double delay_per_byte_ms = 1200.0 / 1200.0;
const double PRINT_INTERVAL = 10.0;

struct Stream {
  bool isDataReadingOut = false;
  uint8_t channel = 0;
};

/*
 * byte: uint8_t byte thats ingested
 * stream: reference to stream state
 * cb: callback f() to process data that is in channel, segment, with
 * segmentData.
 */
template <typename Func>
void processByte(uint8_t byte, Stream &stream, Func cb) {
  if (byte > 0x7f) {
    stream.isDataReadingOut = (byte & 1) == 0;
    stream.channel = ((byte >> 1) & 0x1f) ^ 0x1f;
    if (stream.channel > 31) {
      std::cerr << "Channel out of range\n";
      return;
    }
    if (byte > 190) {
      // blank out screen
      for (int i = 0; i < 8; i++) {
        cb(stream.channel, i, SPACE_ASCII);
      }
    } else if (byte > 169 && byte < 190) {
      // unsure
    }
  } else {
    if (stream.isDataReadingOut) {
      uint8_t segment = (byte & 0xf0) >> 4;
      if (segment > 7) {
        std::cerr << "Segment out of range: " << static_cast<int>(segment)
                  << "\n";
        return;
      }
      uint8_t segmentData = byte & 0x0f;
      if ((stream.channel > 0) && (byte == 0)) {
        segmentData = SPACE_ASCII;
      } else {
        segmentData = (segmentData ^ 0x0f) + 48;
      }
      cb(stream.channel, segment, segmentData);
    }
  }
}

int main(int argc, char *argv[]) {
  Scoreboard scoreboard;
  Stream stream;
  std::string filename;

  for (int i = 1; i < argc; ++i) {
    if ((std::string(argv[i]) == "-i" || std::string(argv[i]) == "--input") &&
        i + 1 < argc) {
      filename = argv[i + 1];
      ++i;
    }
  }

  if (filename.empty()) {
    std::cerr
        << "Input file not specified. Use -i or --input to specify the file.\n";
    return 1;
  }

  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Unable to open file.\n";
    return 1;
  }

  uint8_t byte;
  unsigned int fileOffset = 0;
  auto lastPrint = std::chrono::steady_clock::now();
  std::cout << delay_per_byte_ms << "\n";

  while (file.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
    processByte(byte, stream, [&scoreboard](int c, int s, int v){ scoreboard.update(c, s, v); });
    fileOffset++;
    auto now = std::chrono::steady_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(now - lastPrint).count();
    if (elapsed >= PRINT_INTERVAL) {
      scoreboard.clearConsole();
      scoreboard.print(fileOffset);
      lastPrint = now;
    }

    std::this_thread::sleep_for(
        std::chrono::duration<double, std::milli>(delay_per_byte_ms));
  }
  scoreboard.clearConsole();
  scoreboard.print(fileOffset);
  std::cout << "EOF\n";
  return 0;
}

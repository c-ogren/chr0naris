#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <ratio>
#include <sstream>
#include <thread>

const uint8_t SPACE_ASCII = 0x20;
const double delay_per_byte_ms = 1000.0 / 1200.0;
const double PRINT_INTERVAL = 10.0;

struct Stream {
  bool isDataReadingOut = false;
  uint8_t channel = 0;
};

void processByte(uint8_t byte, uint8_t (&buf)[32][32], Stream &stream) {
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
        buf[stream.channel][i] = SPACE_ASCII;
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
      buf[stream.channel][segment] = segmentData;
    }
  }
}

void clearConsole() {
  std::cout
      << "\x1b[2J\x1b[H"; // ANSI escape to clear screen and move cursor to home
}

void printScoreboardSnapshot(uint8_t (&buf)[32][32], unsigned int fileOffset) {
  for (size_t chan = 0; chan < 32; ++chan) {
    std::ostringstream line;
    line << "#" << std::setw(2) << std::setfill('0') << std::hex << chan
         << ": ";
    for (int seg = 0; seg < 8; ++seg) {
      char c = static_cast<char>(buf[chan][seg]);

      line << ((c == SPACE_ASCII || c == '?') ? ' ' : c);
    }
    line << "'";
    std::cout << line.str() << "\n";
  }
  std::cout << "OFF: " << fileOffset << "\n";
  std::cout << std::flush;
}

int main(int argc, char *argv[]) {
  uint8_t buf[32][32];
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

  for (int i = 0; i < 32; ++i) {
    for (int j = 0; j < 32; ++j) {
      buf[i][j] = SPACE_ASCII;
    }
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
    processByte(byte, buf, stream);
    fileOffset++;
    auto now = std::chrono::steady_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(now - lastPrint).count();
    if (elapsed >= PRINT_INTERVAL) {
      clearConsole();
      printScoreboardSnapshot(buf, fileOffset);
      lastPrint = now;
    }

    std::this_thread::sleep_for(
        std::chrono::duration<double, std::milli>(delay_per_byte_ms));
  }
  clearConsole();
  printScoreboardSnapshot(buf, fileOffset);
  std::cout << "EOF\n";
  return 0;
}

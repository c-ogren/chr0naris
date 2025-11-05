#include "Scoreboard.hpp"
#include "constants.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <ratio>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct Stream {
  bool isDataReadingOut = false;
  uint8_t channel = 0;
};

void sendData(uint8_t *data, size_t len) {
  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

  int c = connect(clientSocket, (struct sockaddr *)&serverAddress,
                  sizeof(serverAddress));

  if (c < 0) {
    std::cerr << "Connection failed\n";
  }

  send(clientSocket, data, len, 0);
  // std::cout << "Message sent" << std::endl;
  close(clientSocket);
}

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
        cb(stream.channel, i, Constants::EMPTY_ASCII);
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
        segmentData = Constants::EMPTY_ASCII;
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
  std::cout << Constants::DELAY_PER_BYTE_MS << "\n";

  while (file.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
    processByte(byte, stream, [&scoreboard](uint8_t c, uint8_t s, uint8_t v) {
      scoreboard.update(c, s, v);
    });
    fileOffset++;
    auto now = std::chrono::steady_clock::now();
    double elapsed =
        std::chrono::duration<double, std::milli>(now - lastPrint).count();
    if (elapsed >= Constants::PRINT_INTERVAL) {
      scoreboard.clearConsole();
      scoreboard.print(fileOffset);
      lastPrint = now;
    }

    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(
        Constants::DELAY_PER_BYTE_MS));
  }
  scoreboard.clearConsole();
  scoreboard.print(fileOffset);
  std::cout << "EOF\n";
  return 0;
}

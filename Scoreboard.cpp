#include "Scoreboard.hpp"
#include "constants.hpp"
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

const short HEIGHT = 32;
const short WIDTH = 8;

Scoreboard::Scoreboard() {
  for (int i = 0; i < HEIGHT; ++i) {
    for (int j = 0; j < WIDTH; ++j) {
      buf[i][j] = Constants::EMPTY_ASCII;
    }
  }
}

void Scoreboard::update(int channel, int segment, char value) {
  buf[channel][segment] = value;
}

void Scoreboard::print(unsigned int fileOffset) const {
  std::cout << "     __________";
  std::cout << "\n";

  for (int i = 0; i < HEIGHT; ++i) {
    std::ostringstream line;
    line << "#" << std::setw(2) << std::setfill('0') << std::hex << i << ": |";
    for (int j = 0; j < WIDTH; ++j) {
      char c = static_cast<char>(buf[i][j]);

      line << ((c == Constants::EMPTY_ASCII || c == '?') ? '-' : c);
    }
    line << "|";
    std::cout << line.str() << "\n";
  }
  
  std::cout << "OFFSET: " << fileOffset << "\n";
  std::cout << std::flush;
}

void Scoreboard::clearConsole() const {
  std::cout << "\x1b[2J\x1b[H"; // ANSI excape to clear the screen and move
                                // cursor home.
}

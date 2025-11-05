#include "Scoreboard.hpp"
#include "constants.hpp"
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

Scoreboard::Scoreboard() {
  for (int i = 0; i < 32; ++i) {
    for (int j = 0; j < 8; ++j) {
      buf[i][j] = Constants::EMPTY_ASCII;
    }
  }
}

void Scoreboard::update(int channel, int segment, char value) {
  buf[channel][segment] = value;
}

void Scoreboard::print(unsigned int fileOffset) const {
  for (int i = 0; i < 32; ++i) {
    std::ostringstream line;
    line << "#" << std::setw(2) << std::setfill('0') << std::hex << i << ": ";
    for (int j = 0; j < 8; ++j) {
      char c = static_cast<char>(buf[i][j]);

      line << ((c == Constants::EMPTY_ASCII || c == '?') ? ' ' : c);
    }
    line << "'";
    std::cout << line.str() << "\n";
  }
  std::cout << "OFFSET: " << fileOffset << "\n";
  std::cout << std::flush;
}

void Scoreboard::clearConsole() const {
  std::cout << "\x1b[2J\x1b[H"; // ANSI excape to clear the screen and move
                                // cursor home.
}

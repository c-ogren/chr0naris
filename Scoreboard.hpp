#pragma once

class Scoreboard {
public:
  Scoreboard();
  void update(int channel, int segment, char value);
  void print(unsigned int fileOffset) const;
  void clearConsole() const;

private:
  char buf[32][8];
};

#pragma once
#include <stdint.h>

namespace game2048 {
static constexpr uint8_t BoardSize = 4;
using Board = uint16_t[BoardSize][BoardSize];
enum class Direction : uint8_t { Up, Right, Down, Left };
struct MoveResult { bool changed; uint16_t mergeValue; uint16_t mergeCount; uint32_t scoreDelta; };

class Game {
 public:
  Game();
  void reset(uint32_t seed);
  MoveResult move(Direction direction);
  void addComboBonus(uint8_t comboMultiplier, uint32_t mergedScore);
  bool undo();
  bool bomb(uint8_t row, uint8_t column);
  bool doubleHighest();
  bool canMove() const;
  uint16_t highestTile() const;
  uint32_t score() const { return score_; }
  const Board& board() const { return board_; }
  uint8_t undoUses() const { return undoUses_; }
  uint8_t bombUses() const { return bombUses_; }
  uint8_t multiplierUses() const { return multiplierUses_; }

 private:
  Board board_{};
  Board previousBoard_{};
  uint32_t score_ = 0;
  uint32_t previousScore_ = 0;
  uint32_t rngState_ = 1;
  bool hasPrevious_ = false;
  uint8_t undoUses_ = 3;
  uint8_t bombUses_ = 2;
  uint8_t multiplierUses_ = 1;
  uint32_t nextRandom();
  void addRandomTile();
  MoveResult moveLine(uint16_t line[BoardSize], bool reverse);
};
}

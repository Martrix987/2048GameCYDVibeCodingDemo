#include "game_logic.h"
namespace game2048 {
Game::Game() { reset(1); }
void Game::reset(uint32_t seed) {
  for (auto& row : board_) for (auto& cell : row) cell = 0;
  score_ = previousScore_ = 0; rngState_ = seed ? seed : 1; hasPrevious_ = false;
  undoUses_ = 3; bombUses_ = 2; multiplierUses_ = 1; addRandomTile(); addRandomTile();
}
uint32_t Game::nextRandom() { rngState_ = rngState_ * 1664525UL + 1013904223UL; return rngState_; }
void Game::addRandomTile() {
  uint8_t empty = 0;
  for (auto& row : board_) for (auto cell : row) empty += cell == 0;
  if (!empty) return;
  uint8_t target = nextRandom() % empty;
  for (auto& row : board_) for (auto& cell : row) if (cell == 0 && target-- == 0) { cell = nextRandom() % 10 == 0 ? 4 : 2; return; }
}
MoveResult Game::moveLine(uint16_t line[BoardSize], bool reverse) {
  uint16_t compacted[BoardSize] = {}, merged[BoardSize] = {}; uint8_t count = 0, out = 0;
  for (uint8_t i = 0; i < BoardSize; ++i) { uint16_t v = line[reverse ? BoardSize - 1 - i : i]; if (v) compacted[count++] = v; }
  MoveResult result{false, 0, 0, 0};
  for (uint8_t i = 0; i < count; ++i) {
    if (i + 1 < count && compacted[i] == compacted[i + 1]) {
      merged[out++] = compacted[i] * 2; result.mergeValue = merged[out - 1]; ++result.mergeCount; result.scoreDelta += merged[out - 1]; ++i;
    } else merged[out++] = compacted[i];
  }
  for (uint8_t i = 0; i < BoardSize; ++i) line[reverse ? BoardSize - 1 - i : i] = merged[i];
  return result;
}
MoveResult Game::move(Direction direction) {
  Board before = {}; for (uint8_t r = 0; r < BoardSize; ++r) for (uint8_t c = 0; c < BoardSize; ++c) before[r][c] = board_[r][c];
  MoveResult total{false, 0, 0, 0};
  for (uint8_t index = 0; index < BoardSize; ++index) {
    uint16_t line[BoardSize] = {}; bool reverse = direction == Direction::Right || direction == Direction::Down;
    for (uint8_t i = 0; i < BoardSize; ++i) line[i] = direction == Direction::Left || direction == Direction::Right ? board_[index][i] : board_[i][index];
    MoveResult result = moveLine(line, reverse); if (result.mergeValue) total.mergeValue = result.mergeValue; total.mergeCount += result.mergeCount; total.scoreDelta += result.scoreDelta;
    for (uint8_t i = 0; i < BoardSize; ++i) { if (direction == Direction::Left || direction == Direction::Right) board_[index][i] = line[i]; else board_[i][index] = line[i]; }
  }
  for (uint8_t r = 0; r < BoardSize; ++r) for (uint8_t c = 0; c < BoardSize; ++c) total.changed |= before[r][c] != board_[r][c];
  if (!total.changed) return total;
  for (uint8_t r = 0; r < BoardSize; ++r) for (uint8_t c = 0; c < BoardSize; ++c) previousBoard_[r][c] = before[r][c];
  previousScore_ = score_; hasPrevious_ = true; score_ += total.scoreDelta; addRandomTile(); return total;
}
void Game::addComboBonus(uint8_t comboMultiplier, uint32_t mergedScore) { if (comboMultiplier > 1) score_ += mergedScore * (comboMultiplier - 1); }
bool Game::undo() {
  if (!hasPrevious_ || !undoUses_) return false;
  for (uint8_t r = 0; r < BoardSize; ++r) for (uint8_t c = 0; c < BoardSize; ++c) board_[r][c] = previousBoard_[r][c];
  score_ = previousScore_; hasPrevious_ = false; --undoUses_; return true;
}
bool Game::bomb(uint8_t row, uint8_t column) { if (!bombUses_ || row >= BoardSize || column >= BoardSize || !board_[row][column]) return false; board_[row][column] = 0; --bombUses_; hasPrevious_ = false; return true; }
bool Game::doubleHighest() {
  if (!multiplierUses_) return false; uint16_t highest = highestTile(); if (!highest) return false;
  if (highest > UINT16_MAX / 2) return false;
  for (auto& row : board_) for (auto& cell : row) if (cell == highest) { cell = highest * 2; --multiplierUses_; hasPrevious_ = false; return true; } return false;
}
uint16_t Game::highestTile() const { uint16_t highest = 0; for (const auto& row : board_) for (auto cell : row) if (cell > highest) highest = cell; return highest; }
bool Game::canMove() const {
  for (uint8_t r = 0; r < BoardSize; ++r) for (uint8_t c = 0; c < BoardSize; ++c) {
    if (!board_[r][c]) return true;
    if (c + 1 < BoardSize && board_[r][c] == board_[r][c + 1]) return true;
    if (r + 1 < BoardSize && board_[r][c] == board_[r + 1][c]) return true;
  } return false;
}
}

#pragma once

#include <cstdint>

// Common constants used across the entire Connect 4 engine
namespace C4 {

constexpr int COLS     = 7;   // board width
constexpr int ROWS     = 6;   // board height
constexpr int COL_BITS = ROWS + 1; // bits per column in the bitboard (6 rows + 1 sentinel)

// Score constants.
// A win in k total moves (counting from move 1) scores (COLS*ROWS + 1 - k) / 2.
// The fastest possible win takes 7 total moves (4 for winner, 3 for loser).
// MAX_SCORE = (42 + 1 - 7) / 2 = 18
constexpr int MAX_SCORE = (COLS * ROWS + 1) / 2 - 3;
constexpr int MIN_SCORE = -MAX_SCORE;

// Column move-ordering: try center columns first for better pruning
constexpr int MOVE_ORDER[COLS] = {3, 2, 4, 1, 5, 0, 6};

// Bit index for cell (col, row) in the bitboard
// Layout: column c occupies bits [c*COL_BITS .. c*COL_BITS + ROWS-1]
//         sentinel bit at c*COL_BITS + ROWS (never part of a valid piece)
inline constexpr int bitIndex(int col, int row) noexcept {
    return col * COL_BITS + row;
}

} // namespace C4

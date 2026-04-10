#pragma once

#include "types.h"

#include <cstdint>
#include <cassert>

// Board represents a Connect 4 position using two 64-bit bitboards.
//
// Encoding:
//   current_ = bitboard of the player whose turn it is
//   mask_    = bitboard of ALL placed pieces (both players)
//
// Column c, row r maps to bit: c * COL_BITS + r
//   (COL_BITS = 7: 6 rows + 1 sentinel above the top row)
//
// The opponent's pieces are: mask_ ^ current_
//
// After play(), current_ becomes the OPPONENT's pieces (next player to move).
class Board {
public:
    Board();
    Board(const Board&) = default;
    Board& operator=(const Board&) = default;

    // ── Queries ──────────────────────────────────────────────────────────────
    bool canPlay(int col) const;
    bool isWinningMove(int col) const;
    bool isDraw() const;

    int  getMoveCount() const       { return moves_; }
    int  getCurrentPlayer() const   { return (moves_ % 2 == 0) ? 1 : 2; }
    int  getHeight(int col) const   { return heights_[col]; }

    // Returns 0 (empty), 1 (player 1), or 2 (player 2) for cell (col, row).
    int  getCell(int col, int row) const;

    // Unique position key for transposition table lookups.
    uint64_t getKey() const { return current_ + mask_; }

    // Raw bitboard accessors (used by the solver for advanced optimisations).
    uint64_t getCurrent()  const { return current_; }
    uint64_t getMask()     const { return mask_;    }

    // ── Mutators ─────────────────────────────────────────────────────────────
    void play(int col);
    void undo();

    // ── Static bitboard helpers ───────────────────────────────────────────────
    // Top row bit of column col (set when the column is full).
    static uint64_t topMask(int col);
    // Bottom row bit of column col (used to seed the "next-piece" formula).
    static uint64_t bottomMask(int col);
    // Bitmask covering all 6 valid rows of column col.
    static uint64_t colMask(int col);

    // Bitmask of bottom row across all columns (used for possibleMoves).
    static constexpr uint64_t BOTTOM_ROW =
        (1ULL << C4::bitIndex(0,0)) | (1ULL << C4::bitIndex(1,0)) |
        (1ULL << C4::bitIndex(2,0)) | (1ULL << C4::bitIndex(3,0)) |
        (1ULL << C4::bitIndex(4,0)) | (1ULL << C4::bitIndex(5,0)) |
        (1ULL << C4::bitIndex(6,0));

    // Bitmask of all valid cells on the board.
    static constexpr uint64_t FULL_BOARD =
        BOTTOM_ROW * ((1ULL << C4::ROWS) - 1);

    // Returns the number of winning-threat cells created by playing in col.
    // Higher = more aggressive move. Used for move ordering in the AI.
    int winThreatsAfterMove(int col) const;

    // Compute all cells where the current player would immediately win.
    uint64_t winningPositions() const;
    // Compute all cells where the opponent would immediately win.
    uint64_t opponentWinningPositions() const;
    // Bitmask of all cells that can be played next turn.
    uint64_t possibleMoves() const;
    // Possible moves that do NOT give the opponent an immediate win.
    uint64_t possibleNonLosingMoves() const;

private:
    uint64_t current_;              // current player's pieces
    uint64_t mask_;                 // all pieces
    int      moves_;                // total moves played so far
    int      heights_[C4::COLS];   // height[c] = number of pieces in column c
    int      history_[C4::COLS * C4::ROWS]; // column played at each half-move

    // Returns true if bitboard pos contains 4-in-a-row.
    static bool hasAlignment(uint64_t pos);

    // Returns bitboard of cells that would complete 4-in-a-row for pieces in pos.
    static uint64_t computeWinPositions(uint64_t pos, uint64_t mask);
};

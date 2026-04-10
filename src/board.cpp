#include "board.h"

using namespace C4;

// ── Construction ─────────────────────────────────────────────────────────────

Board::Board()
    : current_(0), mask_(0), moves_(0)
{
    for (int c = 0; c < COLS; c++) heights_[c] = 0;
}

// ── Static helpers ────────────────────────────────────────────────────────────

uint64_t Board::topMask(int col) {
    // The top row bit of column col is at row ROWS-1.
    return 1ULL << bitIndex(col, ROWS - 1);
}

uint64_t Board::bottomMask(int col) {
    return 1ULL << bitIndex(col, 0);
}

uint64_t Board::colMask(int col) {
    // All ROWS bits for column col.
    return ((1ULL << ROWS) - 1ULL) << (col * COL_BITS);
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool Board::canPlay(int col) const {
    // Column is full when the top row bit is already occupied.
    return heights_[col] < ROWS;
}

bool Board::isWinningMove(int col) const {
    // Would placing a piece in col complete 4-in-a-row for the current player?
    uint64_t newPiece = 1ULL << bitIndex(col, heights_[col]);
    return hasAlignment(current_ | newPiece);
}

bool Board::isDraw() const {
    return moves_ == COLS * ROWS;
}

int Board::getCell(int col, int row) const {
    uint64_t bit = 1ULL << bitIndex(col, row);
    if (!(mask_ & bit)) return 0; // empty

    // Determine which pieces belong to player 1.
    // If moves_ is even  → it is player 1's turn → current_ == player 1's pieces.
    // If moves_ is odd   → it is player 2's turn → current_ == player 2's pieces.
    uint64_t p1 = (moves_ % 2 == 0) ? current_ : (mask_ ^ current_);
    return (p1 & bit) ? 1 : 2;
}

// ── Mutators ──────────────────────────────────────────────────────────────────

void Board::play(int col) {
    history_[moves_] = col;

    // Place piece for current player at (col, heights_[col]).
    uint64_t newPiece = 1ULL << bitIndex(col, heights_[col]);

    // Switch whose pieces "current_" tracks:
    //   current_ XOR mask_ = opponent's old pieces  →  becomes the new current player.
    current_ ^= mask_;

    // Add the new piece to the global mask.
    mask_ |= newPiece;

    heights_[col]++;
    moves_++;
}

void Board::undo() {
    moves_--;
    int col = history_[moves_];
    heights_[col]--;

    uint64_t piece = 1ULL << bitIndex(col, heights_[col]);

    // Remove piece from mask first, then reverse the current_ XOR.
    mask_    ^= piece;
    current_ ^= mask_;
}

// ── Win / move computation ────────────────────────────────────────────────────

bool Board::hasAlignment(uint64_t pos) {
    // Horizontal (columns shift by COL_BITS = 7)
    uint64_t m = pos & (pos >> 7);
    if (m & (m >> 14)) return true;

    // Vertical (rows shift by 1)
    m = pos & (pos >> 1);
    if (m & (m >> 2)) return true;

    // Diagonal top-left → bottom-right (shift by COL_BITS - 1 = 6)
    m = pos & (pos >> 6);
    if (m & (m >> 12)) return true;

    // Diagonal top-right → bottom-left (shift by COL_BITS + 1 = 8)
    m = pos & (pos >> 8);
    if (m & (m >> 16)) return true;

    return false;
}

// Compute all empty cells that would give the given pos a 4-in-a-row.
// mask is the bitmask of ALL occupied cells (used to exclude filled cells).
uint64_t Board::computeWinPositions(uint64_t pos, uint64_t mask) {
    uint64_t r = 0;

    // Vertical
    r |= (pos << 1) & (pos << 2) & (pos << 3);

    // Horizontal
    {
        uint64_t p = (pos << 7) & (pos << 14);
        r |= p & (pos << 21);
        r |= p & (pos >> 7);
        p = (pos >> 7) & (pos >> 14);
        r |= p & (pos << 7);
        r |= p & (pos >> 21);
    }

    // Diagonal (shift 6)
    {
        uint64_t p = (pos << 6) & (pos << 12);
        r |= p & (pos << 18);
        r |= p & (pos >> 6);
        p = (pos >> 6) & (pos >> 12);
        r |= p & (pos << 6);
        r |= p & (pos >> 18);
    }

    // Diagonal (shift 8)
    {
        uint64_t p = (pos << 8) & (pos << 16);
        r |= p & (pos << 24);
        r |= p & (pos >> 8);
        p = (pos >> 8) & (pos >> 16);
        r |= p & (pos << 8);
        r |= p & (pos >> 24);
    }

    // Only empty cells on the board
    return r & (FULL_BOARD ^ mask);
}

int Board::winThreatsAfterMove(int col) const {
    uint64_t bit    = 1ULL << bitIndex(col, heights_[col]);
    uint64_t newPos  = current_ | bit;
    uint64_t newMask = mask_    | bit;
    return __builtin_popcountll(computeWinPositions(newPos, newMask));
}

uint64_t Board::winningPositions() const {
    return computeWinPositions(current_, mask_);
}

uint64_t Board::opponentWinningPositions() const {
    return computeWinPositions(mask_ ^ current_, mask_);
}

uint64_t Board::possibleMoves() const {
    // For each column, the next playable cell = (mask_ + bottomMask(col)) & colMask(col).
    // Doing all columns simultaneously:
    return (mask_ + BOTTOM_ROW) & FULL_BOARD;
}

uint64_t Board::possibleNonLosingMoves() const {
    uint64_t possible    = possibleMoves();
    uint64_t opponentWin = opponentWinningPositions();
    uint64_t forced      = possible & opponentWin;

    if (forced) {
        // If there are two or more forced replies we've already lost — return 0
        // (signalling the caller to detect the loss).
        if (forced & (forced - 1)) return 0;
        // Only the single forced move is allowed.
        possible = forced;
    }

    // Do not play directly below a cell that would give the opponent a win
    // on the very next move.
    return possible & ~(opponentWin >> 1);
}

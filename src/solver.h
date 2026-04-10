#pragma once

#include "board.h"
#include "types.h"

#include <cstdint>
#include <vector>

// Solver implements a perfect-play negamax engine for Connect 4.
//
// Algorithm highlights:
//   • Negamax with alpha-beta pruning
//   • Transposition table (Zobrist-style key, ~8 MB default)
//   • Move ordering: winning moves first, then center-to-edge
//   • "Non-losing moves" pruning: skip moves that hand the opponent a free win
//   • Score = (COLS*ROWS + 1 − total_moves_at_game_end) / 2
//     so the engine prefers winning sooner and losing later
class Solver {
public:
    explicit Solver(int ttSizeBits = 23); // default: 2^23 ≈ 8M entries (~80 MB)

    // Returns the best column (0-indexed) for the current player.
    // If aiFirstCol >= 0 and no move has been made yet, that column is forced.
    int bestMove(Board& board);

    // Full solve: returns the exact score of the position from the current player's
    // perspective.  Positive = current player wins, negative = loses, 0 = draw.
    int solve(Board& board);

    void  resetStats()              { nodeCount_ = 0; }
    long long getNodeCount() const  { return nodeCount_; }

private:
    // ── Transposition table ───────────────────────────────────────────────────
    struct TTEntry {
        uint64_t key;   // position key
        int8_t   val;   // score (in [-MAX_SCORE-1, MAX_SCORE+1])
        // flag: 0 = empty, 1 = lower bound (fail-high), 2 = upper bound (fail-low),
        //        3 = exact
        uint8_t  flag;
    };

    std::vector<TTEntry> tt_;
    int ttMask_;   // tt_.size() - 1 (size is always a power of 2)

    void ttStore(uint64_t key, int val, int flag);
    bool ttLookup(uint64_t key, int& val, int& flag) const;

    // ── Search ────────────────────────────────────────────────────────────────
    long long nodeCount_;

    // Full-depth negamax with alpha-beta pruning.
    int negamax(Board& board, int alpha, int beta);

    // Depth-limited negamax; uses a heuristic evaluation at leaf nodes.
    int negamaxBounded(Board& board, int alpha, int beta, int depth);

    // Heuristic score in (-MAX_SCORE, MAX_SCORE) for non-terminal nodes at depth=0.
    static int heuristic(const Board& board);
};

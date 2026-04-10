#include "solver.h"
#include "types.h"

#include <algorithm>
#include <climits>

using namespace C4;

// ── Construction ─────────────────────────────────────────────────────────────

Solver::Solver(int ttSizeBits)
    : nodeCount_(0)
{
    int size  = 1 << ttSizeBits;
    ttMask_   = size - 1;
    tt_.assign(size, TTEntry{0, 0, 0});
}

// ── Transposition table ───────────────────────────────────────────────────────

void Solver::ttStore(uint64_t key, int val, int flag) {
    TTEntry& e = tt_[key & ttMask_];
    e.key  = key;
    e.val  = static_cast<int8_t>(val);
    e.flag = static_cast<uint8_t>(flag);
}

bool Solver::ttLookup(uint64_t key, int& val, int& flag) const {
    const TTEntry& e = tt_[key & ttMask_];
    if (e.flag == 0 || e.key != key) return false;
    val  = e.val;
    flag = e.flag;
    return true;
}

// ── Negamax ───────────────────────────────────────────────────────────────────

int Solver::negamax(Board& board, int alpha, int beta) {
    nodeCount_++;

    if (board.isDraw()) return 0;

    // Immediate win?
    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (board.canPlay(col) && board.isWinningMove(col))
            return (COLS * ROWS + 1 - board.getMoveCount()) / 2;
    }

    // Tighten upper bound
    int maxScore = (COLS * ROWS - 1 - board.getMoveCount()) / 2;
    if (beta > maxScore) {
        beta = maxScore;
        if (alpha >= beta) return beta;
    }

    // Transposition table
    uint64_t key = board.getKey();
    {
        int ttVal, ttFlag;
        if (ttLookup(key, ttVal, ttFlag)) {
            if (ttFlag == 3) return ttVal;
            if (ttFlag == 1) alpha = std::max(alpha, ttVal);
            if (ttFlag == 2) beta  = std::min(beta,  ttVal);
            if (alpha >= beta) return ttVal;
        }
    }

    // Non-losing moves pruning
    uint64_t moves = board.possibleNonLosingMoves();
    if (moves == 0)
        return -(COLS * ROWS - board.getMoveCount()) / 2;

    const int origAlpha = alpha;
    int best = alpha;

    // Build and sort moves by winning-threat count (most aggressive first).
    // This dramatically improves alpha-beta cutoffs.
    struct ScoredMove { int col; int threats; };
    ScoredMove scored[COLS];
    int nMoves = 0;
    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (!board.canPlay(col)) continue;
        uint64_t cellBit = 1ULL << C4::bitIndex(col, board.getHeight(col));
        if (!(moves & cellBit)) continue;
        scored[nMoves++] = {col, board.winThreatsAfterMove(col)};
    }
    // Insertion sort is fine for ≤7 elements
    for (int i = 1; i < nMoves; i++) {
        ScoredMove tmp = scored[i];
        int j = i - 1;
        while (j >= 0 && scored[j].threats < tmp.threats) { scored[j+1] = scored[j]; j--; }
        scored[j+1] = tmp;
    }

    for (int i = 0; i < nMoves; i++) {
        int col = scored[i].col;

        board.play(col);
        int score = -negamax(board, -beta, -alpha);
        board.undo();

        if (score >= beta) {
            ttStore(key, score, 1);
            return score;
        }
        if (score > best) {
            best  = score;
            alpha = score;
        }
    }

    ttStore(key, best, (best > origAlpha) ? 3 : 2);
    return best;
}

// ── Heuristic evaluation ──────────────────────────────────────────────────────

int Solver::heuristic(const Board& board) {
    // Prefer positions with more winning threats than the opponent.
    // Using popcount of winning positions bitboards as a proxy for strength.
    int myT = __builtin_popcountll(board.winningPositions());
    int opT = __builtin_popcountll(board.opponentWinningPositions());
    return myT - opT;
}

// ── Depth-limited negamax ─────────────────────────────────────────────────────

// Like negamax but stops at depth == 0 and returns the heuristic evaluation.
// Note: this function intentionally does NOT use the transposition table to
// avoid polluting it with heuristic (non-exact) values that would mislead the
// exact negamax solver later.
int Solver::negamaxBounded(Board& board, int alpha, int beta, int depth) {
    nodeCount_++;

    if (board.isDraw()) return 0;

    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (board.canPlay(col) && board.isWinningMove(col))
            return (COLS * ROWS + 1 - board.getMoveCount()) / 2;
    }

    if (depth == 0) return heuristic(board);

    uint64_t moves = board.possibleNonLosingMoves();
    if (moves == 0)
        return -(COLS * ROWS - board.getMoveCount()) / 2;

    int best = alpha;

    // Sort moves by threat count (same as negamax)
    struct ScoredMove { int col; int threats; };
    ScoredMove scored[COLS];
    int nMoves = 0;
    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (!board.canPlay(col)) continue;
        uint64_t cellBit = 1ULL << C4::bitIndex(col, board.getHeight(col));
        if (!(moves & cellBit)) continue;
        scored[nMoves++] = {col, board.winThreatsAfterMove(col)};
    }
    for (int i = 1; i < nMoves; i++) {
        ScoredMove tmp = scored[i];
        int j = i - 1;
        while (j >= 0 && scored[j].threats < tmp.threats) { scored[j+1] = scored[j]; j--; }
        scored[j+1] = tmp;
    }

    for (int i = 0; i < nMoves; i++) {
        int col = scored[i].col;

        board.play(col);
        int score = -negamaxBounded(board, -beta, -alpha, depth - 1);
        board.undo();

        if (score >= beta) return score;
        if (score > best) {
            best  = score;
            alpha = score;
        }
    }

    return best;
}

// ── Public interface ──────────────────────────────────────────────────────────

// Solve using null-window (aspirate) bisection — dramatically faster than a
// single wide-window search because each pass prunes the tree much more
// aggressively with a 1-wide window.
int Solver::solve(Board& board) {
    if (board.isDraw()) return 0;

    // Immediate win?
    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (board.canPlay(col) && board.isWinningMove(col))
            return (COLS * ROWS + 1 - board.getMoveCount()) / 2;
    }

    int min = -(COLS * ROWS - board.getMoveCount()) / 2;
    int max =  (COLS * ROWS + 1 - board.getMoveCount()) / 2;

    while (min < max) {
        int med = min + (max - min) / 2;
        if (med <= 0 && min / 2 < med)      med = min / 2;
        else if (med >= 0 && max / 2 > med) med = max / 2;

        int r = negamax(board, med, med + 1);
        if (r <= med) max = r;
        else          min = r;
    }
    return min;
}

int Solver::bestMove(Board& board) {
    // ── Empty board: the proven optimal first move is the center column ───────
    if (board.getMoveCount() == 0) return 3;

    // ── 1. Immediate win ──────────────────────────────────────────────────────
    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (board.canPlay(col) && board.isWinningMove(col))
            return col;
    }

    // ── 2. Block opponent's immediate win ─────────────────────────────────────
    {
        uint64_t opponentWin = board.opponentWinningPositions();
        uint64_t possible    = board.possibleMoves();
        uint64_t forced      = possible & opponentWin;
        if (forced) {
            for (int i = 0; i < COLS; i++) {
                int col = MOVE_ORDER[i];
                if (!board.canPlay(col)) continue;
                uint64_t bit = 1ULL << C4::bitIndex(col, board.getHeight(col));
                if (forced & bit) return col;
            }
        }
    }

    // ── 3. Choose search strategy based on game phase ─────────────────────────
    // For early positions (many remaining moves), a full exact search is very
    // expensive.  Use a deep heuristic search (depth 16) instead.
    // The exact solver kicks in from move 14 onwards (~28 remaining), where it
    // consistently completes in under 3 seconds.
    const int remaining = COLS * ROWS - board.getMoveCount();
    const bool useExact = (remaining <= 28);

    int bestCol   = -1;
    int bestScore = INT_MIN;

    uint64_t nonLosing = board.possibleNonLosingMoves();

    for (int i = 0; i < COLS; i++) {
        int col = MOVE_ORDER[i];
        if (!board.canPlay(col)) continue;

        uint64_t cellBit = 1ULL << C4::bitIndex(col, board.getHeight(col));
        if (nonLosing && !(nonLosing & cellBit)) continue;

        board.play(col);
        int score;
        if (useExact) {
            score = -solve(board);
        } else {
            // Depth-14 bounded search: handles up to 7 full plies per side,
            // which covers all short-term tactics and threats.
            score = -negamaxBounded(board, -MAX_SCORE, MAX_SCORE, 14);
        }
        board.undo();

        if (score > bestScore) {
            bestScore = score;
            bestCol   = col;
        }
    }

    // Fallback: pick any legal move (should not normally be reached)
    if (bestCol == -1) {
        for (int i = 0; i < COLS; i++) {
            int col = MOVE_ORDER[i];
            if (board.canPlay(col)) { bestCol = col; break; }
        }
    }

    return bestCol;
}

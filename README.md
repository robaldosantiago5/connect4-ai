# Connect 4 — Perfect-Play AI Engine 🎮

A **Velena-style unbeatable Connect 4 AI** written in pure C++17.  
Play in the terminal against an engine that leverages bitboard search, negamax with
alpha-beta pruning, a transposition table, and threat-based move ordering.

---

## Features

### Playable Visual Board
- Colored 7 × 6 grid rendered in the terminal with ANSI codes
- 🔴 Red pieces for Player 1, 🟡 Yellow pieces for Player 2 / AI
- Column numbers displayed for easy input
- Board redrawn after every move

### Flexible Game Modes
- **Human vs AI** — you choose whether to go first or second
- **AI first-move placement** — when the AI opens, you choose which column it plays
- **AI vs AI demo** — watch two perfect-play engines face each other

### Unbeatable AI Engine
| Component | Details |
|---|---|
| **Bitboard representation** | 64-bit integers encode the entire position; O(1) win detection |
| **Negamax + alpha-beta** | Full game-tree search; exact score for every reachable position |
| **Transposition table** | 16 M entries (~160 MB); Zobrist-style key (`current + mask`); exact / lower / upper bound flags |
| **Threat-based move ordering** | Moves sorted by number of new winning threats they create; dramatically improves alpha-beta cut-offs |
| **Non-losing move pruning** | Skips moves that hand the opponent an immediate win |
| **Null-window bisection** | Iterative refinement with 1-wide windows; much faster than a single wide-window search |
| **Depth-14 heuristic (early game)** | Positions with > 28 remaining moves use a 14-ply bounded search with a threat-difference heuristic, keeping response time under 1 second |
| **Exact solver (mid/late game)** | All positions with ≤ 28 remaining moves solved perfectly in milliseconds |

### Perfect Play Awareness
- First move is always the **center column** (proven optimal by game theory)
- Scores positions by *distance to win* — prefers winning sooner over later
- Detects all 4-in-a-row combinations: horizontal, vertical, and both diagonals

---

## Project Structure

```
connect4-ai/
├── README.md          — This file
├── Makefile           — Simple g++ build (C++17, -O2)
└── src/
    ├── types.h        — Constants (COLS, ROWS, MOVE_ORDER, score limits)
    ├── board.h/.cpp   — Bitboard representation, move gen, win detection, undo
    ├── solver.h/.cpp  — Negamax + alpha-beta + TT + bounded heuristic search
    ├── display.h/.cpp — ANSI terminal UI, colored board, input handling
    └── main.cpp       — Game loop, menus, AI vs AI demo
```

---

## Build & Run

**Requirements:** g++ with C++17 support (GCC 7+ or Clang 5+).  
No external libraries — pure standard C++.

```bash
# Clone and build
git clone https://github.com/robaldosantiago5/connect4-ai.git
cd connect4-ai
make          # produces ./connect4

# Play
./connect4
```

Clean the build:
```bash
make clean
```

---

## Gameplay Flow

```
1. Welcome screen
2. Choose mode: Human vs AI  OR  AI vs AI demo
3. Human vs AI:
   a. "Do you want to go first? [y/n]"
   b. If AI goes first: "Which column should AI play first? [1-7]"
4. Game loop:
   a. Board displayed with ANSI colors
   b. Current player's turn (human input OR AI computation)
   c. Win / draw detection after each move
   d. Players switch
5. Game over: winner announced, final board shown
6. "Play again? [y/n]"
```

---

## Performance

| Game phase | Method | Typical response time |
|---|---|---|
| Move 1 (empty board) | Hardcoded center | < 1 ms |
| Early game (> 28 remaining moves) | 14-ply heuristic search | < 700 ms |
| Mid game (≤ 28 remaining moves) | Exact negamax + bisection | < 200 ms |
| Late game (≤ 14 remaining moves) | Exact negamax | < 10 ms |

---

## Technical Notes

### Bitboard Encoding
Each column `c` occupies bits `[c × 7 .. c × 7 + 5]` (6 rows), with bit
`c × 7 + 6` as a sentinel.  The sentinel ensures no carries propagate between
columns during arithmetic operations.

```
col  0   1   2   3   4   5   6
bit  0   7  14  21  28  35  42
     1   8  15  22  29  36  43
     2   9  16  23  30  37  44
     3  10  17  24  31  38  45
     4  11  18  25  32  39  46
     5  12  19  26  33  40  47
  S  6  13  20  27  34  41  48   ← sentinel bits (never set)
```

### Key: `current + mask`
The position key `current_ + mask_` (where `current_` = current player's pieces,
`mask_` = all pieces) is collision-resistant and cheap to compute.

### Score Convention
`score = (COLS × ROWS + 1 − total_moves_at_end) / 2`  
Positive = current player wins, negative = loses, 0 = draw.  
Higher magnitude = faster win/loss.

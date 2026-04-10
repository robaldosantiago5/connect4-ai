#include "board.h"
#include "solver.h"
#include "display.h"
#include "types.h"

#include <iostream>
#include <chrono>
#include <string>

using namespace C4;

// ── Game result helpers ───────────────────────────────────────────────────────

// ── Single game ───────────────────────────────────────────────────────────────

static void playGame(bool humanFirst, int aiOpeningCol, Solver& solver) {
    Board board;

    // Determine which player is human.
    // humanFirst → human = player 1 (moves_ even = player 1)
    int  humanPlayer  = humanFirst ? 1 : 2;
    bool aiUsedOpening = false;

    while (true) {
        Display::clearScreen();
        Display::showBoard(board);

        if (board.isDraw()) {
            Display::showGameOver(0);
            return;
        }

        int currentPlayer = board.getCurrentPlayer();
        bool isHuman      = (currentPlayer == humanPlayer);

        Display::showTurn(currentPlayer, isHuman);

        int col = -1;

        if (isHuman) {
            col = Display::promptColumn(board);
            if (col == -1) return; // EOF
        } else {
            // AI turn
            if (!aiUsedOpening && aiOpeningCol >= 0) {
                // Force the opening column if valid.
                col = aiOpeningCol;
                if (!board.canPlay(col)) {
                    std::cout << "\033[1;31m  Requested opening column is full; AI will choose.\033[0m\n";
                    col = solver.bestMove(board);
                }
                aiUsedOpening = true;
            } else {
                std::cout << "\033[1;33m  AI is thinking...\033[0m\n";
                auto t0 = std::chrono::steady_clock::now();
                col = solver.bestMove(board);
                auto t1 = std::chrono::steady_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "\033[1;33m  AI plays column " << (col + 1)
                          << "  (" << static_cast<long long>(solver.getNodeCount())
                          << " nodes, " << static_cast<int>(ms) << " ms)\033[0m\n";
                solver.resetStats();
            }
        }

        // Check win *before* playing (isWinningMove uses current state).
        bool wins = board.isWinningMove(col);

        board.play(col);

        if (wins) {
            Display::clearScreen();
            Display::showBoard(board);
            Display::showGameOver(currentPlayer);
            return;
        }

        if (board.isDraw()) {
            Display::clearScreen();
            Display::showBoard(board);
            Display::showGameOver(0);
            return;
        }
    }
}

// ── AI vs AI demo ─────────────────────────────────────────────────────────────

static void playAiVsAi(Solver& solver) {
    Board board;
    std::cout << "\033[1;36m  AI vs AI — watching perfect play…\033[0m\n\n";

    while (true) {
        Display::clearScreen();
        Display::showBoard(board);

        if (board.isDraw()) {
            Display::showGameOver(0);
            return;
        }

        int currentPlayer = board.getCurrentPlayer();
        std::cout << "\033[1;33m  AI " << currentPlayer << " is thinking…\033[0m\n";

        solver.resetStats();
        int col   = solver.bestMove(board);
        bool wins = board.isWinningMove(col);

        std::cout << "\033[1;33m  AI " << currentPlayer << " plays column " << (col + 1)
                  << "  (" << solver.getNodeCount() << " nodes)\033[0m\n";

        // Brief pause so the user can follow along.
        std::cout << "  Press Enter to continue…";
        std::string dummy;
        std::getline(std::cin, dummy);

        board.play(col);

        if (wins) {
            Display::clearScreen();
            Display::showBoard(board);
            Display::showGameOver(currentPlayer);
            return;
        }
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main() {
    // Large transposition table: 2^24 ≈ 16M entries (~160 MB).
    // For lower-memory systems change to 23 (~80 MB) or 22 (~40 MB).
    Solver solver(24);

    while (true) {
        Display::showWelcome();

        // ── Mode selection ────────────────────────────────────────────────────
        std::cout << "\033[1;37m  Game modes:\033[0m\n"
                  << "    1. Human vs AI\n"
                  << "    2. AI vs AI (demo)\n\n";

        int mode = 1;
        while (true) {
            std::cout << "\033[1;32m  Choose mode [1-2]: \033[0m";
            std::string line;
            if (!std::getline(std::cin, line)) return 0;
            for (char ch : line) {
                if (ch == '1') { mode = 1; goto modeChosen; }
                if (ch == '2') { mode = 2; goto modeChosen; }
            }
            std::cout << "\033[1;31m  Please enter 1 or 2.\033[0m\n";
        }
        modeChosen:;

        if (mode == 2) {
            playAiVsAi(solver);
        } else {
            // ── Human vs AI ───────────────────────────────────────────────────
            bool humanFirst = Display::promptYesNo("Do you want to go first?");

            int aiOpeningCol = -1;
            if (!humanFirst) {
                aiOpeningCol = Display::promptAnyColumn(
                    "Which column should the AI play first?");
                std::cout << "\033[1;33m  AI will open with column "
                          << (aiOpeningCol + 1) << ".\033[0m\n\n";
            }

            playGame(humanFirst, aiOpeningCol, solver);
        }

        std::cout << "\n";
        bool again = Display::promptYesNo("Play again?");
        if (!again) break;
    }

    std::cout << "\033[1;36m\n  Thanks for playing! Goodbye.\033[0m\n\n";
    return 0;
}

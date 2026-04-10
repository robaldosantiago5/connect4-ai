#include "display.h"
#include "types.h"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <iostream>
#include <string>
#include <limits>

using namespace C4;

// ── ANSI helpers ──────────────────────────────────────────────────────────────

static const char* RESET   = "\033[0m";
static const char* BOLD    = "\033[1m";
static const char* RED     = "\033[1;31m";
static const char* YELLOW  = "\033[1;33m";
static const char* CYAN    = "\033[1;36m";
static const char* GREEN   = "\033[1;32m";
static const char* WHITE   = "\033[1;37m";

// ── Public implementation ─────────────────────────────────────────────────────

void Display::clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

void Display::showWelcome() {
    clearScreen();
    std::cout << CYAN
              << "╔══════════════════════════════════════════════╗\n"
              << "║       CONNECT 4 — PERFECT-PLAY AI ENGINE     ║\n"
              << "║       Velena-style • Bitboard • Negamax       ║\n"
              << "╚══════════════════════════════════════════════╝\n"
              << RESET << "\n";
    std::cout << WHITE << "  " << RED << "●" << WHITE << " = Player 1 (Red)    "
              << YELLOW << "●" << WHITE << " = Player 2 / AI (Yellow)\n\n"
              << RESET;
}

void Display::showBoard(const Board& board) {
    std::cout << "\n";

    // Top border
    std::cout << CYAN << "  ┌";
    for (int c = 0; c < COLS; c++) {
        std::cout << "───";
        if (c < COLS - 1) std::cout << "┬";
    }
    std::cout << "┐\n" << RESET;

    // Rows from top (row ROWS-1) to bottom (row 0)
    for (int r = ROWS - 1; r >= 0; r--) {
        std::cout << CYAN << "  │" << RESET;
        for (int c = 0; c < COLS; c++) {
            int cell = board.getCell(c, r);
            std::cout << " ";
            if      (cell == 1) std::cout << RED    << "●" << RESET;
            else if (cell == 2) std::cout << YELLOW << "●" << RESET;
            else                std::cout << " ";
            std::cout << CYAN << " │" << RESET;
        }
        std::cout << "\n";

        // Row separator (except below the bottom row)
        if (r > 0) {
            std::cout << CYAN << "  ├";
            for (int c = 0; c < COLS; c++) {
                std::cout << "───";
                if (c < COLS - 1) std::cout << "┼";
            }
            std::cout << "┤\n" << RESET;
        }
    }

    // Bottom border
    std::cout << CYAN << "  └";
    for (int c = 0; c < COLS; c++) {
        std::cout << "───";
        if (c < COLS - 1) std::cout << "┴";
    }
    std::cout << "┘\n" << RESET;

    // Column numbers
    std::cout << GREEN << "  ";
    for (int c = 0; c < COLS; c++) {
        std::cout << "  " << (c + 1) << " ";
    }
    std::cout << RESET << "\n\n";
}

void Display::showTurn(int player, bool isHuman) {
    const char* color = (player == 1) ? RED : YELLOW;
    const char* name  = isHuman ? "Human" : "AI";
    std::cout << color << BOLD
              << "  Player " << player << " (" << name << ")'s turn"
              << RESET << "\n";
}

void Display::showGameOver(int winner) {
    std::cout << "\n";
    if (winner == 0) {
        std::cout << CYAN << BOLD
                  << "  ══════════════════════════\n"
                  << "       It's a DRAW!         \n"
                  << "  ══════════════════════════\n"
                  << RESET;
    } else {
        const char* color = (winner == 1) ? RED : YELLOW;
        std::cout << color << BOLD
                  << "  ══════════════════════════\n"
                  << "    Player " << winner << " WINS!  🏆         \n"
                  << "  ══════════════════════════\n"
                  << RESET;
    }
    std::cout << "\n";
}

int Display::promptColumn(const Board& board) {
    while (true) {
        std::cout << GREEN << "  Enter column (1-7): " << RESET;
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF — return sentinel
            return -1;
        }
        // Strip whitespace
        int col = -1;
        for (char ch : line) {
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                col = ch - '0' - 1; // 0-indexed
                break;
            }
        }
        if (col >= 0 && col < COLS && board.canPlay(col))
            return col;

        if (col >= 0 && col < COLS)
            std::cout << RED << "  Column " << (col + 1) << " is full! Try another.\n" << RESET;
        else
            std::cout << RED << "  Invalid input. Please enter a number 1-7.\n" << RESET;
    }
}

bool Display::promptYesNo(const char* question) {
    while (true) {
        std::cout << GREEN << "  " << question << " [y/n]: " << RESET;
        std::string line;
        if (!std::getline(std::cin, line)) return false;
        for (char ch : line) {
            char lo = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (lo == 'y') return true;
            if (lo == 'n') return false;
        }
        std::cout << RED << "  Please enter 'y' or 'n'.\n" << RESET;
    }
}

int Display::promptAnyColumn(const char* prompt) {
    while (true) {
        std::cout << GREEN << "  " << prompt << " [1-7]: " << RESET;
        std::string line;
        if (!std::getline(std::cin, line)) return 3; // default center
        for (char ch : line) {
            if (std::isdigit(static_cast<unsigned char>(ch))) {
                int col = ch - '0' - 1;
                if (col >= 0 && col < C4::COLS) return col;
            }
        }
        std::cout << RED << "  Please enter a number between 1 and 7.\n" << RESET;
    }
}

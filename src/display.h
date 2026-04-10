#pragma once

#include "board.h"

// Display provides all terminal-UI functions for the Connect 4 game.
//
// ANSI colour codes:
//   Player 1 → Bold Red    (\033[1;31m)
//   Player 2 → Bold Yellow (\033[1;33m)
//   Grid     → Bold Cyan   (\033[1;36m)
namespace Display {

// Clears the terminal screen.
void clearScreen();

// Prints the welcome banner.
void showWelcome();

// Renders the current board state with ANSI colours.
void showBoard(const Board& board);

// Prints a status line (whose turn it is).
void showTurn(int player, bool isHuman);

// Prints the game-over message.
void showGameOver(int winner); // 0 = draw, 1 = player 1 wins, 2 = player 2 wins

// Prompts and reads a valid column (1-7) from stdin.
// Returns a 0-indexed column number.  Re-prompts on invalid input.
int  promptColumn(const Board& board);

// Prompts yes/no.  Returns true for 'y'.
bool promptYesNo(const char* question);

// Prompts for a column number in [1..7].  Returns 0-indexed column.
int  promptAnyColumn(const char* prompt);

} // namespace Display

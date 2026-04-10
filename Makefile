CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
TARGET   := connect4
SRCDIR   := src
SRCS     := $(SRCDIR)/main.cpp \
            $(SRCDIR)/board.cpp \
            $(SRCDIR)/solver.cpp \
            $(SRCDIR)/display.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

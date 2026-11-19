# Makefile for divider.c (needs libtiff)
#
# Works on Linux and macOS. On Windows, use WSL (Windows Subsystem for Linux),
# which is Linux, so this same Makefile works there too.
#
# Install the dependencies first:
#   Debian/Ubuntu/WSL: sudo apt install build-essential libtiff-dev pkg-config
#   Fedora:            sudo dnf install gcc make libtiff-devel pkgconf
#   macOS:             xcode-select --install
#                      brew install libtiff pkg-config
#
# Usage:
#   make          builds the program "divider"
#   make clean    removes the program

TARGET = divider
SRC = divider.c
CFLAGS = -Wall -Wextra -O2

# Name of the operating system ("Linux" or "Darwin" for macOS)
UNAME := $(shell uname -s)

# 1st try: ask pkg-config where libtiff is (works on most systems)
TIFF_CFLAGS := $(shell pkg-config --cflags libtiff-4 2>/dev/null)
TIFF_LIBS := $(shell pkg-config --libs libtiff-4 2>/dev/null)

# 2nd try: pkg-config is missing or doesn't know libtiff
ifeq ($(TIFF_LIBS),)
    TIFF_LIBS = -ltiff
    # On macOS, Homebrew installs libraries in a folder the compiler doesn't search by default
    # (/opt/homebrew on Apple Silicon, /usr/local on Intel)
    ifeq ($(UNAME),Darwin)
        BREW_TIFF := $(shell brew --prefix libtiff 2>/dev/null)
        ifneq ($(BREW_TIFF),)
            TIFF_CFLAGS = -I$(BREW_TIFF)/include
            TIFF_LIBS = -L$(BREW_TIFF)/lib -ltiff
        endif
    endif
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(TIFF_CFLAGS) -o $@ $< $(TIFF_LIBS)

clean:
	rm -f $(TARGET)

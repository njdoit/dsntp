# time-shim skeleton (POSIX). On Windows host this is documentation-only unless WSL/Linux cross-build.
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
BIN := dsntp-time-shim

.PHONY: all clean

all: $(BIN)

$(BIN): main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

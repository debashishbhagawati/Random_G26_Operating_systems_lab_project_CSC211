# Makefile for custom UNIX utilities
# Author: Linux Builder Engineer

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
PREFIX  = /usr/local/bin

UTILS   = custom_ls custom_cat custom_grep custom_wc custom_cp custom_mv custom_rm

.PHONY: all install uninstall clean

all: $(UTILS)

custom_ls: src/custom_ls.c
	$(CC) $(CFLAGS) -o $@ $<

custom_cat: src/custom_cat.c
	$(CC) $(CFLAGS) -o $@ $<

custom_grep: src/custom_grep.c
	$(CC) $(CFLAGS) -o $@ $<

custom_wc: src/custom_wc.c
	$(CC) $(CFLAGS) -o $@ $<

custom_cp: src/custom_cp.c
	$(CC) $(CFLAGS) -o $@ $<

custom_mv: src/custom_mv.c
	$(CC) $(CFLAGS) -o $@ $<

custom_rm: src/custom_rm.c
	$(CC) $(CFLAGS) -o $@ $<

install: all
	@echo "Installing to $(PREFIX)..."
	@install -m 755 $(UTILS) $(PREFIX)/
	@echo "Done. All utilities installed to $(PREFIX)"

uninstall:
	@echo "Removing from $(PREFIX)..."
	@cd $(PREFIX) && rm -f $(UTILS)
	@echo "Done."

clean:
	rm -f $(UTILS)
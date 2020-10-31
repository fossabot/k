.POSIX:

PREFIX = /usr/local

BUILD_FLAGS = \
	-std=c99 \
	-D_POSIX_C_SOURCE=200809L \
	-Wall \
	-Wextra \
	-pedantic \
	$(CFLAGS)

BUILD_LDFLAGS = \
	-lcurl \
	$(LDFLAGS)

VALGRIND = \
	--leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--error-exitcode=1 \
	--track-fds=yes \
	--trace-children=yes

OBJ = \
	src/kiss.o \
	src/str.o \
	src/download.o 

kiss: $(OBJ)
	$(CC) -o $@ $(OBJ) $(BUILD_LDFLAGS)

.c.o:
	$(CC) $(BUILD_FLAGS) -g -O0 -c -o $@ $<

check:
	valgrind $(VALGRIND) ./kiss

compdb:
	ninja -t compdb cc > compile_commands.json

clean:
	rm -f kiss $(OBJ)

.PHONY: install clean check

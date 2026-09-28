CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -Ithird_party -Isrc
LDFLAGS := -lm

SRC := src/main.c src/ui.c src/files.c src/png_io.c src/stego.c src/codec.c
OBJ := $(SRC:.c=.o)

TARGET := IU_CODE.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
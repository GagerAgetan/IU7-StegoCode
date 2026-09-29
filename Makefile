CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -Ithird_party -Isrc
LDFLAGS := -lm

CC_WIN  := x86_64-w64-mingw32-gcc
WINDRES := x86_64-w64-mingw32-windres

SRC := src/main.c src/ui.c src/files.c src/png_io.c src/stego.c src/codec.c

# Linux
TARGET := IU_CODE.exe
OBJ := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Windows (кросс-компиляция)
windows: IU_CODE.exe

IU_CODE.exe: $(SRC) app.res
	$(CC_WIN) -std=c11 -O2 -Wall -Wextra -Ithird_party -Isrc $(SRC) app.res -o $@ -lm

app.res: app.rc green_vector.ico
	$(WINDRES) app.rc -O coff -o app.res

clean:
	rm -f $(OBJ) $(TARGET) IU_CODE.exe app.res

.PHONY: all clean windows
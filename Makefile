CC = gcc
CFLAGS = -Wall -Wextra -O2
INCLUDES = 
LIBS = -lole32 -lgdi32 -lwinmm

SRC = main.c
OBJ = $(SRC:.c=.o)

TARGET = pngtubeer.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS) -municode

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -municode

clean:
	rm -fr $(TARGET) $(OBJ)

clean-all:
	rm -fr *.o *.exe

audio: audio.c
	$(CC) $(CFLAGS) -o sound audio.c -lole32 -lwinmm

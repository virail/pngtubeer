CC = gcc
CFLAGS = 
INCLUDES = -I include
LIBS = -L lib -lraylib -lgdi32 -lwinmm

SRC = main.c
OBJ = $(SRC:.c=.o)

TARGET = pngtubeer.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -fr $(TARGET) $(OBJ)

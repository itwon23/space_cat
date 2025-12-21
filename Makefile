CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lpthread -lm -lncursesw -lmpg123
TARGET = SpaceCatGame
SOURCES = main.c Intro.c SpaceCatGame.c PlanetAvoid.c rhythm.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lpthread -lm -lncursesw

TARGET  = SpaceCatGame
SOURCES = main.c Intro.c SpaceCatGame.c PlanetAvoid.c rhythm.c

PACKAGES = build-essential libncursesw5-dev mpg123

.PHONY: all run clean install check_apt

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) -o $(TARGET) $(SOURCES) $(LDFLAGS)

run: install $(TARGET)
	./$(TARGET)

install: check_apt
	@if command -v mpg123 >/dev/null 2>&1; then \
		echo "[install] mpg123 already installed. Skipping apt install."; \
	else \
		echo "[install] Installing dependencies (requires sudo)..."; \
		sudo apt-get update -qq && sudo apt-get install -y $(PACKAGES); \
	fi

clean:
	rm -f $(TARGET)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
LIBS = `pkg-config --cflags --libs gtk+-3.0 gtk-layer-shell-0`
TARGET = nedepanel
SOURCES = panel.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

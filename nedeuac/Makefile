CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE \
         `pkg-config --cflags gtk+-3.0 polkit-gobject-1 polkit-agent-1` \
         -I/usr/include/security
LIBS = `pkg-config --libs gtk+-3.0 polkit-gobject-1 polkit-agent-1` \
       -L/usr/lib/x86_64-linux-gnu -lpam
TARGET = nedeuac
SOURCES = nedeuac.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

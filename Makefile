CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude -pthread
LDFLAGS ?= -lcurl -lpthread -lm
SRCS    = src/main.c src/http.c src/scanner.c src/harvester.c src/output.c src/config.c src/utils.c
OBJ     = $(SRCS:.c=.o)
TARGET  = lowhunt

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/lowhunt
	mkdir -p /usr/share/lowhunt
	cp -r data/* /usr/share/lowhunt/
	mkdir -p /usr/share/lowhunt/platforms
	cp -r platforms/* /usr/share/lowhunt/platforms/

uninstall:
	rm -f /usr/local/bin/lowhunt
	rm -rf /usr/share/lowhunt

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all install uninstall clean

CC      ?= gcc
INCLUDES = -Isrc/include -Isrc/app -Isrc/config -Isrc/core -Isrc/engines -Isrc/net -Isrc/output -Isrc/ui -Isrc/modules
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 $(INCLUDES) -pthread
LDFLAGS ?= -lcurl -lpthread -lm
SRCS    = src/app/main.c src/config/config.c src/core/utils.c src/core/metadata.c src/core/resource_guard.c \
          src/ui/banner.c src/ui/help_menu.c \
          src/net/http.c src/net/scanner.c src/net/harvester.c src/net/harvest_sources.c \
          src/output/output.c src/output/report_bundle.c \
          src/engines/engine_loader.c src/engines/parallel_engine.c src/engines/threadpool_engine.c \
          src/engines/sync_engine.c src/engines/async_engine.c src/engines/fusion_engine.c \
          src/engines/stabilizer_engine.c src/engines/intelligence_engine.c src/engines/brief_report_engine.c
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
	mkdir -p /usr/share/man/man1
	cp man/lowhunt.1 /usr/share/man/man1/

uninstall:
	rm -f /usr/local/bin/lowhunt
	rm -f /usr/share/man/man1/lowhunt.1
	rm -rf /usr/share/lowhunt

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all install uninstall clean

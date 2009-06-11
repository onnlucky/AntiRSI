CC=gcc
CFLAGS=-g -c -Wall `pkg-config --cflags gtk+-2.0` `pkg-config --cflags libpanelapplet-2.0` -std=c99
LDFLAGS=`pkg-config --libs gtk+-2.0` `pkg-config --libs libpanelapplet-2.0` -lXss -lm
PREFIX=/usr

SOURCES=../antirsi-core/antirsi-core.c break-window.c status-display.c
HEADERS=../antirsi-core/antirsi-core.h app.h break-window.h status-display.h antirsi-applet.h
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=antirsi-applet
TEST_EXECUTABLE=antirsi

all: $(SOURCES) $(EXECUTABLE)

test: $(SOURCES) app.c $(TEST_EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

test-run: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) antirsi-applet.o
	$(CC) $(LDFLAGS) $(OBJECTS) antirsi-applet.o -o $@

$(TEST_EXECUTABLE): $(OBJECTS) app.o
	$(CC) $(LDFLAGS) $(OBJECTS) app.o -o $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE) $(TEST_EXECUTABLE) ../antirsi-core/*.o

install:
	cp antirsi-applet $(PREFIX)/lib/gnome-panel/
	cp antirsi-applet.server $(PREFIX)/lib/bonobo/servers/

uninstall:
	rm $(PREFIX)/lib/gnome-panel/antirsi-applet
	rm $(PREFIX)/lib/bonobo/servers/antirsi-applet.server

.PHONY:all run clean install uninstall

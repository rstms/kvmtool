# kvmtool makefile

TARGET = bin/kvmtool.exe

CC = gcc
CC_OPTS := 

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
LIBS = ws2_32

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CC_OPTS) -o $@ $(SOURCES) $(addprefix -l,$(LIBS))

configure:
	@echo "configured"

build: $(TARGET)

run: build
	$(TARGET) -v

debug:
	$(MAKE) CC_OPTS="-O0 -ggdb" $(TARGET)
	gdb $(TARGET)

launch: $(TARGET)
	$(TARGET) -v

clean:
	rm $(TARGET)
	

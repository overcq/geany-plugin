#==============================================================================
.PHONY: build clean rebuild
#==============================================================================
CC := clang
TARGET := utility.so
SOURCES := $(wildcard *.c )
CFLAGS := -fPIC $(shell pkg-config --cflags geany )
LDFLAGS := -shared $(shell pkg-config --libs geany )
#==============================================================================
build: $(TARGET)
$(TARGET): $(SOURCES)
	$(LINK.c) $^ -o $@
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
clean:
	-rm $(TARGET)
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
rebuild: clean build
#==============================================================================

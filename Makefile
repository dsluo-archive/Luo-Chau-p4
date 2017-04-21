GCC = g++
NFLAGS = -lncurses -lform -lmenu
LFLAGS = -Wall -g $(NFLAGS)
CFLAGS = -Wall -std=c++14 -g -O0 -pedantic-errors $(NFLAGS)

TARGET = 1730sh
SRCDIR = ./src
OBJDIR = ./obj

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all dir clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) $(LFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp dir
	$(GCC) $(CFLAGS) -c -o $@ $<

dir:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(TARGET) $(OBJDIR)

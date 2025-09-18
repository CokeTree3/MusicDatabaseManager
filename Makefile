# SRC_DIRS := ./src
# $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Requires the following project directory structure:
#  /bin
#  /obj
#  /src

# Use 'make remove' to clean up the hole project

# Name of target file
TARGET     := test
TARGET_WIN := $(TARGET).exe

CXX        := c++
CXXFLAGS   := -std=c++17 \
             -Wall -Wextra -Werror -Wpointer-arith -Wcast-qual \
             -Wno-missing-braces -Wempty-body -Wno-error=uninitialized \
             -Wno-error=deprecated-declarations \
             -pedantic-errors -pedantic \
             -Os

WINCXX	   := x86_64-w64-mingw32-c++
WINCXXFLAGS:= -static

LD         := c++ -o
LDFLAGS    := -Wall -pedantic

SRCDIR     = src
OBJDIR     = obj
BINDIR     = bin

SOURCES   := $(wildcard $(SRCDIR)/*.cpp)
INCLUDES  := $(wildcard $(SRCDIR)/*.h)
OBJECTS   := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
WINOBJECTS   := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.win.o)

RM         = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LD) $@ $(LDFLAGS) $(OBJECTS)
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

windows: $(BINDIR)/$(TARGET_WIN)

$(BINDIR)/$(TARGET_WIN): $(WINOBJECTS)
	@$(WINCXX) -o $@ $(WINOBJECTS) $(LDFLAGS) $(WINCXXFLAGS)

$(OBJDIR)/%.win.o: $(SRCDIR)/%.cpp
	@$(WINCXX) $(CXXFLAGS) $(WINCXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(WINOBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
	del $(TARGET_DEL) $(OBJS)

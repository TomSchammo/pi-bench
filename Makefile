# C-Bench Makefile
# A high-performance C benchmarking library

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O0 -g
LDFLAGS = -lm -lpthread -lrt

# Directories
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = main.c
HEADERS = bench.h stats.h system.h data_processing.h

# Object files
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/c-bench

# Default target
all: $(TARGET)

# Create directories
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Run the benchmark
run: $(TARGET)
	@echo "Running C-Bench benchmark..."
	@echo "Note: Some features may require root privileges for CPU pinning and real-time scheduling"
	@echo ""
	./$(TARGET)

# Run with sudo (for CPU pinning and real-time scheduling)
run-sudo: $(TARGET)
	@echo "Running C-Bench benchmark with sudo privileges..."
	@echo "This enables CPU pinning and real-time scheduling features"
	@echo ""
	sudo ./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -O0 -g3
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3 -flto
release: $(TARGET)

# Install (copy to /usr/local/bin)
install: $(TARGET)
	@echo "Installing c-bench to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete!"

# Uninstall
uninstall:
	@echo "Removing c-bench from /usr/local/bin..."
	sudo rm -f /usr/local/bin/c-bench
	@echo "Uninstallation complete!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete!"

# Clean and rebuild
rebuild: clean all

# Show help
help:
	@echo "C-Bench Makefile Help"
	@echo "===================="
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the benchmark executable (default)"
	@echo "  run        - Build and run the benchmark"
	@echo "  run-sudo   - Build and run with sudo (enables CPU pinning)"
	@echo "  debug      - Build with debug symbols and no optimization"
	@echo "  release    - Build optimized release version"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  clean      - Remove all build artifacts"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                # Build the benchmark"
	@echo "  make run            # Build and run"
	@echo "  make run-sudo       # Run with CPU pinning features"
	@echo "  make debug          # Build debug version"
	@echo "  make clean          # Clean build files"

# Dependencies
$(OBJDIR)/main.o: main.c bench.h stats.h system.h data_processing.h

# Phony targets
.PHONY: all run run-sudo debug release install uninstall clean rebuild help

# Print build information
info:
	@echo "C-Bench Build Information"
	@echo "========================="
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Libraries: $(LDFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SOURCES)"
	@echo "Headers: $(HEADERS)"

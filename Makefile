# Pi-Bench Makefile
# A high-performance C benchmarking library

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O0 -g
LDFLAGS = -lm -lpthread -lrt

# Directories
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = main.c
HEADERS = $(wildcard include/*.h)

# Object files
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/pi-bench

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

# Install headers to /usr/local/include/pi-bench
install:
	@echo "Installing pi-bench headers to /usr/local/include/pi-bench/..."
	sudo mkdir -p /usr/local/include/pi-bench
	sudo cp $(HEADERS) /usr/local/include/pi-bench/
	@echo "Installation complete!"

# Uninstall
uninstall:
	@echo "Removing pi-bench headers from /usr/local/include/pi-bench/..."
	sudo rm -rf /usr/local/include/pi-bench
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
	@echo "Pi-Bench Makefile Help"
	@echo "======================"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build the benchmark executable (default)"
	@echo "  run        - Build and run the benchmark"
	@echo "  run-sudo   - Build and run with sudo (enables CPU pinning)"
	@echo "  debug      - Build with debug symbols and no optimization"
	@echo "  release    - Build optimized release version"
	@echo "  install    - Install headers to /usr/local/include/pi-bench"
	@echo "  uninstall  - Remove headers from /usr/local/include/pi-bench"
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
$(OBJDIR)/main.o: main.c include/bench.h include/stats.h include/system.h include/data_processing.h

# Phony targets
.PHONY: all run run-sudo debug release install uninstall clean rebuild help

# Print build information
info:
	@echo "Pi-Bench Build Information"
	@echo "=========================="
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Libraries: $(LDFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SOURCES)"
	@echo "Headers: $(HEADERS)"

# ==============================================================================
# VORPAL Build & Test Convenience Targets
# ==============================================================================

.PHONY: all build test test-verbose test-quick clean help

# Default target
all: build

# Build everything with multi-instance support enabled
build:
	@echo "Building VORPAL with multi-instance support..."
	@mkdir -p build
	@cd build && cmake -DENABLE_LIBPD_MULTI=ON .. && make -j$$(nproc)

# Build and run all tests (recommended)
test: build
	@echo "Running all tests..."
	@cd build && ctest --output-on-failure

# Show detailed test output
test-verbose: build
	@echo "Running all tests (verbose)..."
	@cd build && ctest -V

# Run tests without rebuilding (fast iteration)
test-quick:
	@echo "Running tests (no rebuild)..."
	@cd build && ctest --output-on-failure

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@rm -rf build

# Show available targets
help:
	@echo "VORPAL Build Targets:"
	@echo "  make build        - Build all targets"
	@echo "  make test         - Build and run all tests"
	@echo "  make test-verbose - Run tests with full output"
	@echo "  make test-quick   - Run tests without rebuilding"
	@echo "  make clean        - Remove build directory"
	@echo ""
	@echo "Advanced CTest Usage (from build/):"
	@echo "  ctest -j4              - Run tests in parallel"
	@echo "  ctest -R instance      - Run only tests matching 'instance'"
	@echo "  ctest --rerun-failed   - Re-run only failed tests"

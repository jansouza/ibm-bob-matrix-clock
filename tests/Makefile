# Smart Matrix Clock — native host test suite
#
# No hardware needed. Compiles pure-logic modules against a thin stub layer
# and runs them on the host (Linux/macOS) with a minimal test runner.
#
# Usage:
#   make          — build + run all tests
#   make build    — build only (no run)
#   make clean    — remove build artefacts

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic \
            -I../smart-matrix-clock-esp32 \
            -I stubs \
            -DHOST_TEST \
            -g

SRC_DIR  := ../smart-matrix-clock-esp32
STUB_DIR := stubs
BUILD    := build

# Production source files compiled for host (no Arduino deps in these)
PROD_SRCS := \
    $(SRC_DIR)/text_encoding.cpp \
    $(SRC_DIR)/locale_data.cpp

# Test source files
TEST_SRCS := \
    test_text_encoding.cpp \
    test_locale_data.cpp \
    test_slot_schedule.cpp \
    test_data_fetcher_split.cpp \
    test_persistence_language.cpp \
    runner.cpp

# Stub source file for definitions used across multiple test units
STUB_SRCS := $(STUB_DIR)/stubs.cpp

ALL_SRCS  := $(PROD_SRCS) $(TEST_SRCS) $(STUB_SRCS)
ALL_OBJS  := $(patsubst %.cpp,$(BUILD)/%.o,$(notdir $(ALL_SRCS)))

BINARY    := $(BUILD)/run_tests

# ─── vpath lets make find sources in multiple directories ─────────────────────
vpath %.cpp $(SRC_DIR) $(STUB_DIR) .

# ─── Default target ───────────────────────────────────────────────────────────
.PHONY: all build clean

all: build run

build: $(BINARY)

run: $(BINARY)
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo " Running Smart Matrix Clock native tests"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@./$(BINARY)
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

$(BUILD):
	mkdir -p $(BUILD)

$(BINARY): | $(BUILD)
$(BINARY): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)

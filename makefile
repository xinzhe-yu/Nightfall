# ============================================================================
# Nightfall C2 Framework — Build System
# ============================================================================
# Two binaries: server (operator-side) and implant (target-side)
# Both link against shared code in common/
#
# Usage:
#   make            — build both binaries
#   make server     — build server only
#   make implant    — build implant only
#   make clean      — remove all build artifacts
#   make debug      — build with debug symbols and no optimization
# ============================================================================

# --- Compiler Configuration ---
CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -pedantic -std=c2x

# --- Directory Layout ---
# Build artifacts go in build/ so your source tree stays clean.
# Each component gets its own subdir so object files from different
# components can't collide (both have main.c → main.o).
BUILD_DIR      = build
SERVER_BUILD   = $(BUILD_DIR)/server
IMPLANT_BUILD  = $(BUILD_DIR)/implant
COMMON_BUILD   = $(BUILD_DIR)/common

# --- Include Paths ---
# -I flags tell the preprocessor where to find #include "..." files.
# Each binary needs its own headers PLUS the shared common headers.
COMMON_INCLUDES  = -Icommon/include
SERVER_INCLUDES  = -Iserver/include $(COMMON_INCLUDES)
IMPLANT_INCLUDES = -Iimplant/include $(COMMON_INCLUDES)

# --- Source Files ---
# Append new .c files here as you build them out.
COMMON_SRC  = common/src/logging.c \
              common/src/protocol.c \
              common/src/types.c

SERVER_SRC  = server/src/main.c \
              server/src/session.c \
              server/src/listener.c \
              # server/src/cli.c

IMPLANT_SRC = implant/src/main.c \
              # implant/src/transport.c \
              # implant/src/tasking.c \
              # implant/src/beacon.c

# --- Object Files ---
# We want .o files in build/, not next to sources.
#   $(notdir ...)          strips directory → just filename
#   $(patsubst %.c,%.o, )  swaps extension
#   $(addprefix ...)       prepends build directory
#
# Example: common/src/logging.c → build/common/logging.o
COMMON_OBJ  = $(addprefix $(COMMON_BUILD)/, $(patsubst %.c,%.o,$(notdir $(COMMON_SRC))))
SERVER_OBJ  = $(addprefix $(SERVER_BUILD)/, $(patsubst %.c,%.o,$(notdir $(SERVER_SRC))))
IMPLANT_OBJ = $(addprefix $(IMPLANT_BUILD)/, $(patsubst %.c,%.o,$(notdir $(IMPLANT_SRC))))

# --- Binary Outputs ---
SERVER_BIN  = $(BUILD_DIR)/nightfall_server
IMPLANT_BIN = $(BUILD_DIR)/nightfall_implant

# ============================================================================
# Targets
# ============================================================================

# 'all' is the default (first non-special target). Bare 'make' hits this.
all: $(SERVER_BIN) $(IMPLANT_BIN)

# --- Link Targets ---
# '|' is an order-only prerequisite: ensures directory exists but
# doesn't trigger rebuilds when directory timestamp changes.
$(SERVER_BIN): $(SERVER_OBJ) $(COMMON_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ) $(COMMON_OBJ)

$(IMPLANT_BIN): $(IMPLANT_OBJ) $(COMMON_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(IMPLANT_OBJ) $(COMMON_OBJ)

# --- Compile Rules (per-component) ---
# Each component needs its own pattern rule because they have different
# -I flags. A single generic rule wouldn't know which includes to use.
#
#   $@ = target (.o file being built)
#   $< = first prerequisite (.c file)
#   -c = compile only, don't link

$(COMMON_BUILD)/%.o: common/src/%.c | $(COMMON_BUILD)
	$(CC) $(CFLAGS) $(COMMON_INCLUDES) -c $< -o $@

$(SERVER_BUILD)/%.o: server/src/%.c | $(SERVER_BUILD)
	$(CC) $(CFLAGS) $(SERVER_INCLUDES) -c $< -o $@

$(IMPLANT_BUILD)/%.o: implant/src/%.c | $(IMPLANT_BUILD)
	$(CC) $(CFLAGS) $(IMPLANT_INCLUDES) -c $< -o $@

# --- Directory Creation ---
$(BUILD_DIR) $(COMMON_BUILD) $(SERVER_BUILD) $(IMPLANT_BUILD):
	mkdir -p $@

# --- Debug Build ---
# Target-specific variable: appends to CFLAGS ONLY for this invocation.
# -g: debug symbols  -O0: no optimization  -DDEBUG: defines DEBUG macro
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean all

# --- Convenience Targets ---
server: $(SERVER_BIN)
implant: $(IMPLANT_BIN)

clean:
	rm -rf $(BUILD_DIR)

# .PHONY: these aren't real files. Without this, a file named 'clean'
# would make 'make clean' say "already up to date."
.PHONY: all clean debug server implant
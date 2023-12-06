# ANSI color codes
RED   = \033[1;31m
GREEN = \033[1;32m
BLUE  = \033[1;34m
CYAN  = \033[1;36m
RESET = \033[0m

# Source and object files
SRCS = $(wildcard *.c)
TEMP_SRCS = $(wildcard *temp*.c)
OFILES = $(SRCS:.c=.o)
TEST = sorber_basic_test.c
OFILES := $(filter-out $(TEMP_SRCS:.c=.o), $(OFILES))

# Compiler and flags
CC = gcc
ifeq ($(MAKECMDGOALS),test)
CFLAGS = -Wall -g -O0 -fPIC
else
CFLAGS = -Wall -g -O0 -fPIC
endif
NDEBUG_FLAG = -DNDEBUG

INCLUDES = -I./

# Compile .c to .o
.c.o:
	@echo "$(BLUE)Compiling $<...$(RESET)"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Linking to create shared library
notjustcats: $(OFILES)
	@echo "$(GREEN)Linking $@...$(RESET)"
	$(CC) $(LDFLAGS) -o $@ $(OFILES)

# Default target
all: clean notjustcats
	@echo "$(GREEN)Build complete.$(RESET)"

# Clean up
clean:
	@echo "$(RED)Cleaning up...$(RESET)"
	rm -rf *.o notjustcats


# Debug target
debug: clean all test
	@echo "$(CYAN)Debug build complete.$(RESET)"

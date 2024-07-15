# ®emilskorov for multithreading functionality
# the executables will be built in the "build" directory.
# the executables can be run in a terminal/shell
# cmd: ./pathto/executable
# for tests where files have to be passed as arguments
# cmd: ./pathto/executable pathto/file1 pathto/file2
# to run tests with all the outputs from built test executables add VERBOSE=1 as argument behind make test -> "make tests VERBOSE=1"

# Default value for VERBOSE and N (amount)
VERBOSE ?= 0
N ?= 1
THREADS ?= 1

# Directories
SRC_DIR = src
TEST_DIR = test
TEST_SUBDIRS = $(shell find $(TEST_DIR) -type d)
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(foreach dir, $(TEST_SUBDIRS), $(wildcard $(dir)/*.c))

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Target
TEST_TARGET = $(foreach test_src, $(TEST_SRCS), $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(test_src)))

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) -pthread -fsanitize=address -g -gdwarf-4

# Default rule
all: $(TEST_TARGET)

# Rule for compiling test source files into test targets
$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(OBJS) | $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(OBJS) $< -o $@

# Rule for compiling source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create build subsdirectories if they don't exist
$(foreach dir, $(TEST_SUBDIRS), $(shell mkdir -p $(patsubst $(TEST_DIR)/%, $(BUILD_DIR)/%, $(dir))))

# Test execution rule
test_exec:
	@if [ "$(TEST_FILE)" = "$(BUILD_DIR)/test_unthreaded_no_wrap/test_by_file" ]; then \
		if [ "$(VERBOSE)" = "0" ]; then \
			if ./$(TEST_FILE) ./test/test_unthreaded_no_wrap/testfile.txt ./test/test_unthreaded_no_wrap/testfile_output.txt > /dev/null 2>&1; then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed! \033[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed. \033[0m"; \
				echo failed >> .test_results; \
			fi; \
		else \
			if ./$(TEST_FILE) ./test/test_unthreaded_no_wrap/testfile.txt ./test/test_unthreaded_no_wrap/testfile_output.txt; then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed! \033[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed. \033[0m"; \
				echo failed >> .test_results; \
			fi; \
		fi; \
	elif [ "$(TEST_FILE)" = "$(BUILD_DIR)/test_unthreaded_wrap/test_by_file" ]; then \
		if [ "$(VERBOSE)" = "1" ]; then \
			if ./$(TEST_FILE) ./test/test_unthreaded_wrap/testfile.txt ./test/test_unthreaded_wrap/testfile_output.txt; then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed33[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed.\033[0m"; \
				echo failed >> .test_results; \
			fi; \
		else \
			if ./$(TEST_FILE) ./test/test_unthreaded_wrap/testfile.txt ./test/test_unthreaded_wrap/testfile_output.txt > /dev/null 2>&1; then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed!\033[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed. \033[0m"; \
				echo failed >> .test_results; \
			fi; \
		fi; \
	else \
		if [ "$(VERBOSE)" = "1" ]; then \
			if ./$(TEST_FILE); then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed!\033[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed.\033[0m"; \
				echo failed >> .test_results; \
			fi; \
		else \
			if ./$(TEST_FILE) > /dev/null 2>&1; then \
				echo "-> $(TEST_FILE) \033[0;32m -> Passed!\033[0m"; \
				echo passed >> .test_results; \
			else \
				echo "-> $(TEST_FILE) \033[0;31m -> Failed.\033[0m"; \
				echo failed >> .test_results; \
			fi; \
		fi; \
	fi

# Test rule to run all test executables
tests: $(TEST_TARGET)
	@echo ----------------------------------------------
	@echo
	@echo RUNNING TESTS...
	@echo
	@rm -f .test_results; \
	export VERBOSE=$(VERBOSE); \
	echo $(TEST_TARGET) | tr ' ' '\n' | parallel -j $(THREADS) --halt soon,fail=1 '$(MAKE) test_exec TEST_FILE={}' ; \
	passed=$$(grep -c passed .test_results); \
	failed=$$(grep -c failed .test_results); \
	total=$$((passed + failed)); \
	echo "$$failed" > .failed_count; \
	{ \
		echo " "; \
		echo "Summary: From $$total tests run"; \
		if [ $$passed -ne 0 ]; then \
			echo "         \033[0;32m$$passed passed\033[0m"; \
		else \
			echo -n "         $$passed passed"; \
		fi; \
		if [ $$failed -ne 0 ]; then \
			echo "         \033[0;31m$$failed failed\033[0m"; \
		else \
			echo "         $$failed failed"; \
		fi; \
	}

# Rule to run tests repeatedly in parallel
tests_repeatedly: all
	@rm -f .failed_count .test_results .total_failed; \
	start_time=$$(date +%s); \
	echo " "; \
	echo "Testing began at: $$(date)"; \
	for i in $(shell seq 1 $(N)); do \
		$(MAKE) tests THREADS=$(THREADS) VERBOSE=$(VERBOSE); \
		current_failed=$$(cat .failed_count 2>/dev/null || echo 0); \
		echo $$current_failed >> .total_failed; \
	done; \
	total_failed=$$(awk '{s+=$$1} END {print s}' .total_failed 2>/dev/null || echo 0); \
	echo " "; \
	echo "-----------------------------------"; \
	if [ "$$total_failed" -ne "0" ]; then \
		echo "Total failed tests over $(N) runs: \033[0;31m$$total_failed\033[0m"; \
	else \
		echo "Total failed tests over $(N) runs: $$total_failed"; \
	fi; \
	end_time=$$(date +%s); \
	elapsed_time=$$((end_time - start_time)); \
	echo " "; \
	echo "Testing finished at: $$(date)"; \
	echo "Time elapsed: $$elapsed_time seconds"; \
	echo " "; \
	rm -f .failed_count .test_results .total_failed

# Define phony targets
.PHONY: test tests_repeatedly test_exec

# Clean up
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

.PHONY: pack
pack:
	zip -r submission.zip src/

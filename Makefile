# ®emilskorov for multithreading functionality
# the executables will be built in the "build" directory.
# the executables can be run in a terminal/shell
# cmd: ./pathto/executable
# for tests where files have to be passed as arguments
# cmd: ./pathto/executable pathto/file1 pathto/file2
# to run tests with all the outputs from built test executables add VERBOSE=1 as argument behind make test -> "make tests VERBOSE=1"

# Default value for VERBOSE, N (amount), THREADS, SANITIZE, and ASAN_OPTIONS_ENABLED
VERBOSE ?= 0
N ?= 1
THREADS ?= 1
SANITIZE ?= 0
ASAN_OPTIONS_ENABLED ?= 0

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
CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) -pthread -g -gdwarf-4
ifeq ($(SANITIZE), 1)
    CFLAGS += -fsanitize=address
endif

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

# Create build subdirectories if they don't exist
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
				echo "-> $(TEST_FILE) \033[0;32m -> Passed!\033[0m"; \
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
test_all: $(TEST_TARGET)
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
test_all_repeat: all
	@rm -f .failed_count .test_results .total_failed; \
	start_time=$$(date +%s); \
	echo " "; \
	echo "Testing began at: $$(date)"; \
	for i in $(shell seq 1 $(N)); do \
		$(MAKE) test_all THREADS=$(THREADS) VERBOSE=$(VERBOSE); \
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

# Individual test rules
test_utnowrap_byfile: $(BUILD_DIR)/test_unthreaded_no_wrap/test_by_file
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_no_wrap/test_by_file

test_utwrap_byfile: $(BUILD_DIR)/test_unthreaded_wrap/test_by_file
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_wrap/test_by_file

test_utnowrap_complex: $(BUILD_DIR)/test_unthreaded_no_wrap/test_complex
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_no_wrap/test_complex

test_utwrap_complex: $(BUILD_DIR)/test_unthreaded_wrap/test_complex
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_wrap/test_complex

test_utnowrap_simple: $(BUILD_DIR)/test_unthreaded_no_wrap/test_simple
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_no_wrap/test_simple

test_utwrap_simple: $(BUILD_DIR)/test_unthreaded_wrap/test_simple
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unthreaded_wrap/test_simple

test_threaded: $(BUILD_DIR)/test_threaded/test
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_threaded/test

test_unit_read: $(BUILD_DIR)/test_unit/test_read
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unit/test_read

test_unit_write: $(BUILD_DIR)/test_unit/test_write
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_unit/test_write

test_daemon: $(BUILD_DIR)/test_daemon/test
	$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/test_daemon/test

# Rule to run tests repeatedly for a specific test
test_repeat:
	@rm -f .failed_count .test_results .total_failed; \
	start_time=$$(date +%s); \
	echo " "; \
	echo "Testing began at: $$(date)"; \
	for i in $(shell seq 1 $(N)); do \
		$(MAKE) test_exec TEST_FILE=$(BUILD_DIR)/$(TEST) THREADS=$(THREADS) VERBOSE=$(VERBOSE); \
		current_failed=$$(cat .failed_count 2>/dev/null || echo 0); \
		echo "Iteration $$i: $$current_failed failed"; \
		echo $$current_failed >> .total_failed; \
	done; \
	total_failed=$$(awk '{s+=$$1} END {print s}' .total_failed 2>/dev/null || echo 0); \
	end_time=$$(date +%s); \
	elapsed_time=$$((end_time - start_time)); \
	echo " "; \
	echo "Testing ended at: $$(date)"; \
	echo "Total failed tests: $$total_failed"; \
	echo "Total elapsed time: $$elapsed_time seconds"; \
	if [ $$total_failed -eq 0 ]; then \
		echo "All tests passed!"; \
	else \
		echo "Some tests failed."; \
	fi

# Help command
help:
	@echo ""
	@echo "\033[1;31mThe following dependency is required:\033[0m"
	@echo "  - parallel"
	@echo "use command \033[1m'make help_parallel'\033[0m for installation instructions"
	@echo ""
	@echo "\033[1;31mAvailable commands:\033[0m"
	@echo ""
	@echo "  \033[1;33mmake \033[1;36mall\033[0m                      - Build all test targets"
	@echo "  \033[1;33mmake \033[1;36mclean\033[0m                    - Clean up build directory"
	@echo "  \033[1;33mmake \033[1;36mpack\033[0m                     - Pack source files into submission.zip"
	@echo "  \033[1;33mmake \033[1;32mtest_all\033[0m                 - Run all tests"
	@echo "  \033[1;33mmake \033[1;32mtest_all_repeat\033[0m          - Run all tests repeatedly"
	@echo "  \033[1;33mmake \033[1;32mtest_repeat\033[0m              - Run a specific test repeatedly - \033[1;31m'make help_test_repeat'\033[0m for more information"
	@echo "  \033[1;33mmake \033[1;32mtest_utnowrap_byfile\033[0m     - Run unthreaded no wrap by file test"
	@echo "  \033[1;33mmake \033[1;32mtest_utwrap_byfile\033[0m       - Run unthreaded wrap by file test"
	@echo "  \033[1;33mmake \033[1;32mtest_utnowrap_complex\033[0m    - Run unthreaded no wrap complex test"
	@echo "  \033[1;33mmake \033[1;32mtest_utwrap_complex\033[0m      - Run unthreaded wrap complex test"
	@echo "  \033[1;33mmake \033[1;32mtest_utnowrap_simple\033[0m     - Run unthreaded no wrap simple test"
	@echo "  \033[1;33mmake \033[1;32mtest_utwrap_simple\033[0m       - Run unthreaded wrap simple test"
	@echo "  \033[1;33mmake \033[1;32mtest_unit_read\033[0m           - Run unit read test"
	@echo "  \033[1;33mmake \033[1;32mtest_unit_write\033[0m          - Run unit write test"
	@echo "  \033[1;33mmake \033[1;32mtest_threaded\033[0m            - Run threaded test"
	@echo "  \033[1;33mmake \033[1;32mtest_daemon\033[0m              - Run daemon test"
	@echo ""
	@echo "\033[1mArguments:\033[0m"
	@echo ""
	@echo "  \033[1;33mVERBOSE\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m                   - Enable verbose output \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled"
	@echo "  \033[1;33mN\033[0m=\033[1;32m<number>\033[0m                    - Number of repetitions for test_all_repeat"
	@echo "  \033[1;33mTHREADS\033[0m=\033[1;32m<number>\033[0m              - Number of threads for parallel execution"
	@echo "  \033[1;33mTEST\033[0m=\033[1;32m<test>\033[0m                   - Test to run repeatedly - Can only be used with test_all_repeat"
	@echo "  \033[1;33mSANITIZE\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m                  - Enable address sanitizer flag (\033[1;42m-fsanitize=address\033[0m) \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled - default is disabled"
	@echo "  \033[1;33mASAN_OPTIONS_ENABLED\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m      - Enable \033[1;41mASAN_OPTIONS=detect_leaks=1\033[0m flag \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled - default is disabled"
	@echo ""

# Parallel-specific help command
help_parallel:
	@echo ""
	@echo "\033[34mInstalling parallel on \033[32mUbuntu/Debian\033[0m"
	@echo "> sudo apt-get update"
	@echo "> sudo apt-get -y install parallel"
	@echo ""
	@echo "\033[34mInstalling parallel on \033[33mFedora\033[0m"
	@echo "> sudo dnf makecache --refresh"
	@echo "> sudo dnf -y install parallel"
	@echo ""
	@echo "\033[34mInstalling parallel on \033[35mCentOS\033[0m"
	@echo "> sudo yum makecache"
	@echo "> sudo yum -y install parallel"
	@echo ""
	@echo "\033[34mInstalling parallel on \033[36mArch Linux\033[0m"
	@echo "> sudo pacman -Syu"
	@echo "> sudo pacman -S parallel"
	@echo ""
	@echo "\033[34mInstalling parallel on \033[1;37mmacOS\033[0m"
	@echo "> brew install parallel"
	@echo ""
	@echo "\033[34mInstalling parallel on \033[31mWindows\033[0m"
	@echo "https://www.gnu.org/software/parallel/parallel_tutorial.html"
	@echo "Or use Cygwin terminal to install parallel"
	@echo "apt-cyg install parallel"
	@echo ""

# test_repeat help command
help_test_repeat:
	@echo ""
	@echo "\033[1;31mUsage:\033[0m"
	@echo "> \033[1;33mmake \033[1;32mtest_repeat\033[0m \033[1;33mTEST\033[0m=\033[1;32m<test>\033[0m \033[1;33mN\033[0m=\033[1;32m<number>\033[0m \033[1;33mTHREADS\033[0m=\033[1;32m<number>\033[0m \033[1;33mVERBOSE\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m"
	@echo ""
	@echo "\033[1;31mExample Usage:\033[0m"
	@echo "> \033[1;33mmake \033[1;32mtest_repeat\033[0m \033[1;33mTEST\033[0m=\033[1;32mtest_unthreaded_no_wrap/test_by_file\033[0m \033[1;33mN\033[0m=\033[1;32m10\033[0m \033[1;33mTHREADS\033[0m=\033[1;32m4\033[0m \033[1;33mVERBOSE\033[0m=\033[1;32m1\033[0m"
	@echo ""
	@echo "We pass the 'TEST' argument the test name - can be found under '/test' dir or '/build' dir if built - without the '.c' extension"
	@echo "Then after '/' we pass the test file name"
	@echo "The file needs to be compiled before running the test"

# Default rule for undefined targets
.DEFAULT:
	@echo ""
	@echo "\033[1;31mError: Unknown command '\033[1;33m$@\033[1;31m'.\033[0m"
	@echo "Use \033[1;33mmake help\033[0m for a list of available commands."
	@echo ""

# Define phony targets
.PHONY: all clean pack help help_parallel test test_all test_all_repeat test_repeat test_exec test_utnowrap_byfile test_utwrap_byfile test_utnowrap_complex test_utwrap_complex test_utnowrap_simple test_utwrap_simple test_threaded test_unit_read test_unit_write test_daemon

# Clean up
clean:
	rm -rf $(BUILD_DIR)

pack:
	zip -r submission.zip src/

# ®emilskorov for multithreading functionality.
# ®yasirjumaah UX design, help commands, Valgrind testing logic.
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
ASAN ?= 0

# Valgrind arguments
ifeq ($(VERBOSE), 1)
VALGRIND_ARGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose
else
VALGRIND_ARGS = --leak-check=full --show-leak-kinds=all --track-origins=yes
endif

# Directories
SRC_DIR = src
TEST_DIR = test
TEST_SUBDIRS = $(shell find $(TEST_DIR) -type d)
INCLUDE_DIR = include
BUILD_DIR = build
LOG_DIR = logs

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
ifeq ($(SANITIZE), 1)
	CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) -pthread -fsanitize=address -g -gdwarf-4
else ifeq ($(SANITIZE), 0)
	CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) -pthread -g -gdwarf-4
endif

# Default rule
all: $(TEST_TARGET)

# Rule for compiling test source files into test targets
$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(OBJS) | $(BUILD_DIR)
ifeq ($(ASAN), 1)
	ASAN_OPTIONS=detect_leaks=1 $(CC) $(CFLAGS) $(OBJS) $< -o $@
else
	$(CC) $(CFLAGS) $(OBJS) $< -o $@
endif

# Rule for compiling source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

${LOG_DIR}:
	mkdir -p ${LOG_DIR}

# Create build subdirectories if they don't exist
$(foreach dir, $(TEST_SUBDIRS), $(shell mkdir -p $(patsubst $(TEST_DIR)/%, $(BUILD_DIR)/%, $(dir))))
${shell mkdir -p ${LOG_DIR}}

# Valgrind test rule to run all test executables and log results
test_valgrind: $(TEST_TARGET)
	@echo "\n----------------------------------------------"
	@echo "\033[1;36mVALGRIND TESTING STARTED\033[0m"
	@echo "----------------------------------------------\n"
	@rm -f .valgrind_results; \
	animate() { \
		trap "exit" INT TERM; \
		while :; do \
			printf "\rRunning Valgrind Tests 🕛   "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕐.  "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕑.. "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕒..."; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕓   "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕔.  "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕕.. "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕖..."; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕗   "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕘.  "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕙.. "; sleep 0.5; \
			printf "\rRunning Valgrind Tests 🕚..."; sleep 0.5; \
		done; \
	}; \
	animate & \
	ANIMATE_PID=$$!; \
	trap "kill $$ANIMATE_PID; exit" INT TERM; \
	for test in $(TEST_TARGET); do \
		class_name=$$(echo $$test | sed -E 's|$(BUILD_DIR)/test_([^/]+)/.*|\1|'); \
		test_name=$$(basename $$test); \
		log_file=$(LOG_DIR)/$$class_name-test_$$test_name.log; \
		valgrind $(VALGRIND_ARGS) $$test > $$log_file 2>&1; \
		printf "\r\033[K"; \
		if grep -q "ERROR SUMMARY: 0 errors" $$log_file; then \
			printf "\033[1;36m$$class_name/$$test_name \033[1;34m-> \033[1;32mPASSED\033[0m\n\n"; \
		else \
			printf "\033[1;31m$$class_name/$$test_name -> FAILED\033[0m\n\n"; \
			grep "definitely lost" $$log_file; \
			grep "indirectly lost" $$log_file; \
			grep "possibly lost" $$log_file; \
		fi; \
	done; \
	kill $$ANIMATE_PID; \
	wait $$ANIMATE_PID 2>/dev/null;
	@echo "\n----------------------------------------------"
	@echo "\033[1;36mVALGRIND TESTING COMPLETED\033[0m"
	@echo "----------------------------------------------\n"



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
	echo $(TEST_TARGET) | tr ' ' '\n' | parallel -j $(THREADS) --halt soon,fail=1 '$(MAKE) --no-print-directory test_exec TEST_FILE={}' ; \
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
		$(MAKE) --no-print-directory test_all THREADS=$(THREADS) VERBOSE=$(VERBOSE); \
		current_failed=$$(cat .failed_count 2>/dev/null || echo 0); \
		echo $$current_failed >> .total_failed; \
	done; \
	total_failed=$$(awk '{s+=$$1} END {print s}' .total_failed 2>/dev/null || echo 0); \
	total_passed=$$(grep -c passed .test_results); \
	echo " "; \
	echo "-----------------------------------"; \
	if [ "$$total_failed" -ne "0" ]; then \
		echo "Total passed \033[0;32mtests\033[0m over \033[1;33m$(N) runs: \033[0;32m$$((N * total_passed - total_failed))\033[0m"; \
		echo "Total \033[1;31mfailed\033[0m tests over \033[1;33m$(N) runs: \033[0;31m$$total_failed\033[0m"; \
	else \
		echo "Total \033[0;32mpassed\033[0m tests over \033[1;33m$(N)\033[0m runs: \033[0;32m$$((N * total_passed - total_failed))\033[0m"; \
		echo "Total \033[1;31mfailed\033[0m tests over \033[1;33m$(N)\033[0m runs: \033[1;31m$$total_failed\033[0m"; \
	fi; \
	end_time=$$(date +%s); \
	elapsed_time=$$((end_time - start_time)); \
	echo " "; \
	echo "Testing finished at: $$(date)"; \
	echo "Time elapsed: $$elapsed_time seconds"; \
	echo " "; \
	rm -f .failed_count .test_results .total_failed


# Rule to run tests repeatedly for a specific test
test_repeat:
	@rm -f .failed_count .test_results .total_failed; \
	start_time=$$(date +%s); \
	echo " "; \
	echo "Testing began at: $$(date)"; \
	for i in $(shell seq 1 $(N)); do \
		$(MAKE) --no-print-directory test_exec TEST_FILE=$(BUILD_DIR)/$(TEST) THREADS=$(THREADS) VERBOSE=$(VERBOSE); \
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

# Help command
help:
	@echo ""
	@echo "\033[1;31mThe following dependencies are required:\033[0m"
	@echo "  - zip"
	@echo "  - parallel"
	@echo "use command \033[1m'make help_dep'\033[0m for installation instructions"
	@echo ""
	@echo "\033[1mAvailable commands:\033[0m"
	@echo ""
	@echo "  \033[1;33mmake \033[1;36mhelp\033[0m                     - Display help information"
	@echo "  \033[1;33mmake \033[1;36mhelp_args\033[0m                - Display arguments usage instructions"
	@echo "  \033[1;33mmake \033[1;36mhelp_dep\033[0m                 - Display parallel installation instructions"
	@echo "  \033[1;33mmake \033[1;36mhelp_test_repeat\033[0m         - Display test repeat usage instructions"
	@echo ""
	@echo "  \033[1;33mmake \033[1;37mall\033[0m                      - Build all test targets"
	@echo "  \033[1;33mmake \033[1;37mclean\033[0m                    - Clean up build directory"
	@echo "  \033[1;33mmake \033[1;37mclean_logs\033[0m               - Clean up logs directory"
	@echo "  \033[1;33mmake \033[1;37mpack\033[0m                     - Pack source files into submission.zip"
	@echo "  \033[1;33mmake \033[1;37mclean_pack\033[0m               - Clean up build and logs directory and pack source files"
	@echo "                                - it will only pack \033[1mdaemon.c\033[0m and \033[1mringbuf.c\033[0m"
	@echo "                                - it will ignore all other files inside the \033[1m/src\033[0m directory"
	@echo ""
	@echo "  \033[1;33mmake \033[1;34mtest_all\033[0m                 - Run all tests"
	@echo "  \033[1;33mmake \033[1;34mtest_all_repeat\033[0m          - Run all tests repeatedly"
	@echo "  \033[1;33mmake \033[1;34mtest_repeat\033[0m              - Run a specific test repeatedly - \033[1;31m'make help_test_repeat'\033[0m for more information"
	@echo "  \033[1;33mmake \033[1;34mtest_valgrind\033[0m            - Run all tests with valgrind for memory leak detection - options: VERBOSE=1|0"
	@echo ""
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

# Parallel-specific help command
help_dep:
	@echo ""
	@echo "-- \033[1;31mDependency Installation Instructions\033[0m --"
	@echo ""
	@echo "\033[34mInstalling package on \033[32mUbuntu/Debian\033[0m"
	@echo "> sudo apt-get update"
	@echo "> sudo apt-get -y install parallel zip"
	@echo ""
	@echo "\033[34mInstalling package on \033[33mFedora\033[0m"
	@echo "> sudo dnf makecache --refresh"
	@echo "> sudo dnf -y install parallel zip"
	@echo ""
	@echo "\033[34mInstalling package on \033[35mCentOS\033[0m"
	@echo "> sudo yum makecache"
	@echo "> sudo yum -y install parallel zip"
	@echo ""
	@echo "\033[34mInstalling package on \033[36mArch Linux\033[0m"
	@echo "> sudo pacman -Syu"
	@echo "> sudo pacman -S parallel zip"
	@echo ""
	@echo "\033[34mInstalling package on \033[1;37mmacOS\033[0m"
	@echo "> brew install parallel zip"
	@echo ""
	@echo "\033[34mInstalling a package on \033[31mWindows\033[0m"
	@echo "\033[1mParallel -> \033[0mhttps://www.gnu.org/software/parallel/parallel_tutorial.html"
	@echo "\033[1mzip -> \033[0mShould be included in windows 11 by default - if not then you are on your own :) - or just manually package the files"
	@echo "Or use \033[31mCygwin\033[0m terminal to install packages"
	@echo "> apt-cyg install parallel zip"
	@echo ""
	@echo "-- \033[1;31mEnd of Dependency Installation Instructions\033[0m --"
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
	@echo "The 'N' argument is the number of times the test will be run"
	@echo "The 'THREADS' argument is the number of threads to be used for parallel execution"
	@echo "The 'VERBOSE' argument is used to enable verbose output"
	@echo ""

# help_args command
help_args:
	@echo ""
	@echo "\033[1mBuild Arguments:\033[0m"
	@echo ""
	@echo "  \033[1;33mSANITIZE\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m                  - Enable address sanitizer flag (\033[1;42m-fsanitize=address\033[0m) \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled (default: 0)"
	@echo "  \033[1;33mASAN\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m                      - Enable \033[1;41mASAN_OPTIONS=detect_leaks=1\033[0m flag \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled (default: 0)"
	@echo ""
	@echo "\033[1mTest Arguments:\033[0m"
	@echo ""
	@echo "  \033[1;33mVERBOSE\033[0m=\033[1;32m1\033[0m/\033[1;34m0\033[0m                   - Enable verbose output \033[1;31m1\033[0m=enabled, \033[1;34m0\033[0m=disabled (default: 0)"
	@echo "  \033[1;33mN\033[0m=\033[1;32m<number>\033[0m                    - Number of repetitions for test_all_repeat (default: 1)"
	@echo "  \033[1;33mTHREADS\033[0m=\033[1;32m<number>\033[0m              - Number of threads for parallel execution (default: 1)"
	@echo "  \033[1;33mTEST\033[0m=\033[1;32m<test>\033[0m                   - Test to run repeatedly - Can only be used with test_repeat"
	@echo ""

# Default rule for undefined targets
.DEFAULT:
	@echo ""
	@echo "\033[1;31mError: Unknown command '\033[1;33m$@\033[1;31m'.\033[0m"
	@echo "Use \033[1;33mmake help\033[0m for a list of available commands."
	@echo ""

# Define phony targets
.PHONY: all clean clean_logs clean_pack pack help help_dep help_test_repeat help_args test test_all test_all_repeat test_repeat test_exec test_utnowrap_byfile test_utwrap_byfile test_utnowrap_complex test_utwrap_complex test_utnowrap_simple test_utwrap_simple test_threaded test_unit_read test_unit_write test_daemon

# Clean up
clean:
	rm -rf $(BUILD_DIR)

# Clean up logs
clean_logs:
	rm -rf $(LOG_DIR)

# Clean up everything and pack source files
clean_pack: clean clean_logs pack

# Pack source files
pack:
	zip -r submission.zip src/daemon.c src/ringbuf.c

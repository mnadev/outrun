# Makefile for off-target unit tests
# Compiles and runs tests on the host machine using Unity framework

CC = cc
CFLAGS = -Wall -Wextra -g -I./support -I./unity -I../src/c

# Source files
UNITY_SRC = unity/unity.c
PACE_ENGINE_SRC = ../src/c/pace_engine.c
RUN_STATE_SRC = ../src/c/run_state.c
FEATURES_SRC = ../src/c/features.c
ALERT_ENGINE_SRC = ../src/c/alert_engine.c
PLAN_SRC = ../src/c/plan.c
HAPTIC_PATTERNS_SRC = ../src/c/haptic_patterns.c
WATCH_SRC = ../src/c/watch.c
FEEDBACK_SRC = ../src/c/feedback.c
HR_STATS_SRC = ../src/c/hr_stats.c
SETTINGS_SRC = ../src/c/settings.c
MOCK_WATCH_SRC = support/mock_watch.c
PERSIST_STUB_SRC = support/persist_stub.c

# Test executables
TEST_PACE_ENGINE = test_pace_engine_runner
TEST_RUN_STATE = test_run_state_runner
TEST_FEATURES = test_features_runner
TEST_ALERT_ENGINE = test_alert_engine_runner
TEST_PLAN = test_plan_runner
TEST_HAPTIC_PATTERNS = test_haptic_patterns_runner
TEST_FEEDBACK = test_feedback_runner
TEST_HR_STATS = test_hr_stats_runner
TEST_SETTINGS = test_settings_runner

.PHONY: all test clean

all: test

test: $(TEST_PACE_ENGINE) $(TEST_RUN_STATE) $(TEST_FEATURES) $(TEST_ALERT_ENGINE) $(TEST_PLAN) $(TEST_HAPTIC_PATTERNS) $(TEST_FEEDBACK) $(TEST_HR_STATS) $(TEST_SETTINGS)
	@echo "========================================="
	@echo "Running Pace Engine Tests..."
	@echo "========================================="
	@./$(TEST_PACE_ENGINE)
	@echo ""
	@echo "========================================="
	@echo "Running Run State Tests..."
	@echo "========================================="
	@./$(TEST_RUN_STATE)
	@echo ""
	@echo "========================================="
	@echo "Running Features Tests..."
	@echo "========================================="
	@./$(TEST_FEATURES)
	@echo ""
	@echo "========================================="
	@echo "Running Alert Engine Tests..."
	@echo "========================================="
	@./$(TEST_ALERT_ENGINE)
	@echo ""
	@echo "========================================="
	@echo "Running Plan Engine Tests..."
	@echo "========================================="
	@./$(TEST_PLAN)
	@echo ""
	@echo "========================================="
	@echo "Running Haptic Pattern Tests..."
	@echo "========================================="
	@./$(TEST_HAPTIC_PATTERNS)
	@echo ""
	@echo "========================================="
	@echo "Running Feedback Routing Tests..."
	@echo "========================================="
	@./$(TEST_FEEDBACK)
	@echo ""
	@echo "========================================="
	@echo "Running HR Stats Tests..."
	@echo "========================================="
	@./$(TEST_HR_STATS)
	@echo ""
	@echo "========================================="
	@echo "Running Settings Tests..."
	@echo "========================================="
	@./$(TEST_SETTINGS)
	@echo ""
	@echo "========================================="
	@echo "All tests complete!"
	@echo "========================================="

$(TEST_PACE_ENGINE): test_pace_engine.c $(PACE_ENGINE_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_RUN_STATE): test_run_state.c $(RUN_STATE_SRC) $(WATCH_SRC) $(MOCK_WATCH_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_FEATURES): test_features.c $(FEATURES_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_ALERT_ENGINE): test_alert_engine.c $(ALERT_ENGINE_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_PLAN): test_plan.c $(PLAN_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_HAPTIC_PATTERNS): test_haptic_patterns.c $(HAPTIC_PATTERNS_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_FEEDBACK): test_feedback.c $(FEEDBACK_SRC) $(WATCH_SRC) $(HAPTIC_PATTERNS_SRC) $(MOCK_WATCH_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_HR_STATS): test_hr_stats.c $(HR_STATS_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_SETTINGS): test_settings.c $(SETTINGS_SRC) $(PERSIST_STUB_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_PACE_ENGINE) $(TEST_RUN_STATE) $(TEST_FEATURES) $(TEST_ALERT_ENGINE) $(TEST_PLAN) $(TEST_HAPTIC_PATTERNS) $(TEST_FEEDBACK) $(TEST_HR_STATS) $(TEST_SETTINGS)
	rm -rf *.dSYM

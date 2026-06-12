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

# Test executables
TEST_PACE_ENGINE = test_pace_engine_runner
TEST_RUN_STATE = test_run_state_runner
TEST_FEATURES = test_features_runner
TEST_ALERT_ENGINE = test_alert_engine_runner
TEST_PLAN = test_plan_runner

.PHONY: all test clean

all: test

test: $(TEST_PACE_ENGINE) $(TEST_RUN_STATE) $(TEST_FEATURES) $(TEST_ALERT_ENGINE) $(TEST_PLAN)
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
	@echo "All tests complete!"
	@echo "========================================="

$(TEST_PACE_ENGINE): test_pace_engine.c $(PACE_ENGINE_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_RUN_STATE): test_run_state.c $(RUN_STATE_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -DRUN_STATE_HOST_TEST -o $@ $^

$(TEST_FEATURES): test_features.c $(FEATURES_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_ALERT_ENGINE): test_alert_engine.c $(ALERT_ENGINE_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_PLAN): test_plan.c $(PLAN_SRC) $(UNITY_SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_PACE_ENGINE) $(TEST_RUN_STATE) $(TEST_FEATURES) $(TEST_ALERT_ENGINE) $(TEST_PLAN)
	rm -rf *.dSYM

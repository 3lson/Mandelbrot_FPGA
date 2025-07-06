#include "base_testbench.h"
#include <cstdint>
#include <verilated_cov.h>
#include <gtest/gtest.h>
#include <bit>

unsigned int ticks = 0;

// Convert double to Q4.28 fixed point format (matching the new module's format)
int32_t double_to_fixed_point(double val) {
    return static_cast<int32_t>(val * (1LL << 28));
}

class MandelbrotCalculatorTestbench : public BaseTestbench {
protected:
    void clockCycle() {
        top->clk = 0;
        top->eval();
        #ifndef __APPLE__
        tfp->dump(2 * ticks);
        #endif

        top->clk = 1;
        top->eval();
        #ifndef __APPLE__
        tfp->dump(2 * ticks + 1);
        #endif
        ticks++;
    }

    void initializeInputs() override {
        top->rst = 1;
        top->start = 0;
        top->c_re = 0;
        top->c_im = 0;
        top->max_iter = 0;
    }

    void resetDUT() {
        top->rst = 1;
        clockCycle();
        top->rst = 0;
        clockCycle();
        ASSERT_EQ(top->ready, 1);
        ASSERT_EQ(top->iterations, 0);
    }

    uint32_t run_test(double c_real, double c_imag, uint32_t max_iter) {
        // Wait for ready state
        while (!top->ready) {
            clockCycle();
        }

        // Set up inputs
        top->c_re = double_to_fixed_point(c_real);
        top->c_im = double_to_fixed_point(c_imag);
        top->max_iter = max_iter;
        
        // Start calculation
        top->start = 1;
        clockCycle();
        top->start = 0;

        // Wait for completion with timeout
        int timeout = (max_iter + 10) * 3; // Extra margin for pipeline delays
        while (!top->ready && timeout > 0) {
            clockCycle();
            timeout--;
        }

        EXPECT_NE(timeout, 0) << "Simulation timed out!";
        
        uint32_t final_iter_count = top->iterations;

        // Verify ready state is maintained
        clockCycle();
        EXPECT_EQ(top->ready, 1) << "DUT did not maintain ready state after completion.";

        return final_iter_count;
    }
};

// Test 1: c = 0 + 0i
// This point is in the set and should run until max_iterations.
TEST_F(MandelbrotCalculatorTestbench, PointInSet_Origin) {
    resetDUT();
    const uint32_t max_iter = 100;
    uint32_t result = run_test(0.0, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}

// Test 2: c = -1 + 0i
// This point is also in the set (it oscillates) and should run until max_iterations.
TEST_F(MandelbrotCalculatorTestbench, PointInSet_NegativeOne) {
    resetDUT();
    const uint32_t max_iter = 150;
    uint32_t result = run_test(-1.0, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}

// Test 3: c = 2 + 0i
// This point is outside the set and should escape quickly.
TEST_F(MandelbrotCalculatorTestbench, PointOutsideSet_Simple) {
    resetDUT();
    const uint32_t max_iter = 100;
    uint32_t result = run_test(2.0, 0.0, max_iter);
    EXPECT_LT(result, 5); // Should escape quickly
}

// Test 4: c = 1 + 0i
// This point is outside the set.
TEST_F(MandelbrotCalculatorTestbench, PointOutsideSet_One) {
    resetDUT();
    const uint32_t max_iter = 100;
    uint32_t result = run_test(1.0, 0.0, max_iter);
    EXPECT_LT(result, 10); // Should escape relatively quickly
}

// Test 5: c = 0.3 + 0.6i
// This point is clearly outside the set and should escape relatively quickly.
TEST_F(MandelbrotCalculatorTestbench, PointOutsideSet_Complex) {
    resetDUT();
    const uint32_t max_iter = 100;
    uint32_t result = run_test(0.3, 0.6, max_iter);
    EXPECT_LT(result, max_iter); // Should escape before max iterations
    EXPECT_GT(result, 0); // Should take at least one iteration
    EXPECT_LT(result, 20); // Should escape within reasonable iterations
}

// Test 6: Test with very low max_iter to verify early termination
TEST_F(MandelbrotCalculatorTestbench, LowMaxIterations) {
    resetDUT();
    const uint32_t max_iter = 5;
    uint32_t result = run_test(0.0, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}

// Test 7: Test a point that's clearly outside the set
// c = -2 + 0i is definitely outside the set
TEST_F(MandelbrotCalculatorTestbench, PointOutsideSet_NegativeTwo) {
    resetDUT();
    const uint32_t max_iter = 50;
    uint32_t result = run_test(-2.0, 0.0, max_iter);
    EXPECT_LT(result, 10); // Should escape quickly
}

// Test 8: Test multiple consecutive calculations
TEST_F(MandelbrotCalculatorTestbench, ConsecutiveCalculations) {
    resetDUT();
    const uint32_t max_iter = 50;
    
    // First calculation - point in set
    uint32_t result1 = run_test(0.0, 0.0, max_iter);
    EXPECT_EQ(result1, max_iter);
    
    // Second calculation - point outside set
    uint32_t result2 = run_test(2.0, 0.0, max_iter);
    EXPECT_LT(result2, 5);
    
    // Third calculation - another point in set
    uint32_t result3 = run_test(-1.0, 0.0, max_iter);
    EXPECT_EQ(result3, max_iter);
}

// Test 9: Test the boundary behavior more carefully
// c = -0.5 + 0i is on the boundary and converges to a fixed point
TEST_F(MandelbrotCalculatorTestbench, PointOnBoundary_FixedPoint) {
    resetDUT();
    const uint32_t max_iter = 50;
    uint32_t result = run_test(-0.5, 0.0, max_iter);
    // This point converges to a fixed point, so it should reach max_iter
    EXPECT_EQ(result, max_iter);
}

// Test 10: Test a point that's definitely in the main cardioid
// c = -0.1 + 0i is inside the main cardioid
TEST_F(MandelbrotCalculatorTestbench, PointInMainCardioid) {
    resetDUT();
    const uint32_t max_iter = 100;
    uint32_t result = run_test(-0.1, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}
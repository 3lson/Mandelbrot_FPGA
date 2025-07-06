#include "base_testbench.h"
#include <cstdint>
#include <verilated_cov.h>
#include <gtest/gtest.h>
#include <bit>

unsigned int ticks = 0;

int32_t double_to_q4_28(double val) {
    return static_cast<int32_t>(val * (1LL << 28));
}

class MandelbrotTestbench : public BaseTestbench {
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
        top->c_real_in = 0;
        top->c_imag_in = 0;
        top->max_iterations_in = 0;
    }

    void resetDUT() {
        top->rst = 1;
        clockCycle();
        top->rst = 0;
        clockCycle();
        ASSERT_EQ(top->done, 0);
        ASSERT_EQ(top->iteration_count_out, 0);
    }

    uint16_t run_test(double c_real, double c_imag, uint16_t max_iter) {
        top->c_real_in = double_to_q4_28(c_real);
        top->c_imag_in = double_to_q4_28(c_imag);
        top->max_iterations_in = max_iter;
        
        top->start = 1;
        clockCycle();
        top->start = 0;

        int timeout = max_iter + 10;
        while (!top->done && timeout > 0) {
            clockCycle();
            timeout--;
        }

        EXPECT_NE(timeout, 0) << "Simulation timed out!";
        
        uint16_t final_iter_count = top->iteration_count_out;

        clockCycle();
        EXPECT_EQ(top->done, 0) << "DUT did not return to idle state after completion.";

        return final_iter_count;
    }
};

// Test 1: c = 0 + 0i
// This point is in the set and should run until max_iterations.
TEST_F(MandelbrotTestbench, PointInSet_Origin) {
    resetDUT();
    const uint16_t max_iter = 100;
    uint16_t result = run_test(0.0, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}

// Test 2: c = -1 + 0i
// This point is also in the set (it oscillates) and should run until max_iterations.
TEST_F(MandelbrotTestbench, PointInSet_NegativeOne) {
    resetDUT();
    const uint16_t max_iter = 150;
    uint16_t result = run_test(-1.0, 0.0, max_iter);
    EXPECT_EQ(result, max_iter);
}

// Test 3: c = 2 + 0i
// This point is outside the set and should escape immediately.
// z0=0, z1=c=2. |z1|^2 = 4. Escape condition is |z|^2 >= 4.
// It takes one iteration to check z0 and determine z1.
// In the next cycle, it checks z1 and escapes. So it should take 1 iteration.
TEST_F(MandelbrotTestbench, PointOutsideSet_Simple) {
    resetDUT();
    const uint16_t max_iter = 100;
    uint16_t result = run_test(2.0, 0.0, max_iter);
    EXPECT_EQ(result, 1);
}

// Test 4: c = 1 + 0i
// This point is outside the set.
// z0=0, z1=1, z2=2, z3=5. Escapes on the 3rd iteration check.
// iter 0: z=0
// iter 1: z=1
// iter 2: z=2. |z|^2=4. Escapes.
// Should take 2 iterations.
TEST_F(MandelbrotTestbench, PointOutsideSet_One) {
    resetDUT();
    const uint16_t max_iter = 100;
    uint16_t result = run_test(1.0, 0.0, max_iter);
    EXPECT_EQ(result, 2);
}

// Test 5: c = 0.25 + 0.5i
// A slightly more complex point outside the set.
// z0 = 0
// z1 = 0.25 + 0.5i
// z2 = (0.25^2 - 0.5^2) + 0.25  + i*(2*0.25*0.5 + 0.5) = 0.0625 + i*0.75
// |z2|^2 = 0.0625^2 + 0.75^2 = 0.0039 + 0.5625 = 0.5664 < 4
// z3 = ... will eventually escape. This test verifies it finishes.
TEST_F(MandelbrotTestbench, PointOutsideSet_Complex) {
    resetDUT();
    const uint16_t max_iter = 100;
    uint16_t result = run_test(0.25, 0.5, max_iter);
    EXPECT_EQ(result, 6); // Fail expected
}
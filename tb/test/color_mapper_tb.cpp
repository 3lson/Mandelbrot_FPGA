#include "base_testbench.h"
#include <cstdint>
#include <verilated_cov.h>
#include <gtest/gtest.h>

unsigned int ticks = 0;

class ColorMapperTestbench : public BaseTestbench {
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
        top->iterations_in = 0;
        top->max_iter = 100;
    }

    void resetDUT() {
        // No reset signal for this module, just initialize
        initializeInputs();
        clockCycle(); // Give one clock cycle for outputs to settle
    }

    struct ColorResult {
        uint8_t r, g, b;
    };

    ColorResult run_color_test(uint32_t iterations, uint32_t max_iter) {
        // Set inputs
        top->iterations_in = iterations;
        top->max_iter = max_iter;
        
        // Clock once to process the input
        clockCycle();
        
        // Return the color output
        return {
            static_cast<uint8_t>(top->r),
            static_cast<uint8_t>(top->g),
            static_cast<uint8_t>(top->b)
        };
    }
};

// Test 1: Point in Mandelbrot set (iterations >= max_iter) should be black
TEST_F(ColorMapperTestbench, PointInSet_Black) {
    resetDUT();
    
    ColorResult result = run_color_test(100, 100);
    EXPECT_EQ(result.r, 0);
    EXPECT_EQ(result.g, 0);
    EXPECT_EQ(result.b, 0);
}

// Test 2: Point in set with iterations > max_iter should still be black
TEST_F(ColorMapperTestbench, PointInSet_ExcessIterations) {
    resetDUT();
    
    ColorResult result = run_color_test(150, 100);
    EXPECT_EQ(result.r, 0);
    EXPECT_EQ(result.g, 0);
    EXPECT_EQ(result.b, 0);
}

// Test 3: First color range (0 <= iterations < 255) - Red gradient
TEST_F(ColorMapperTestbench, ColorRange1_RedGradient) {
    resetDUT();
    
    // Test beginning of range
    ColorResult result1 = run_color_test(0, 1000);
    EXPECT_EQ(result1.r, 0);
    EXPECT_EQ(result1.g, 0);
    EXPECT_EQ(result1.b, 0);
    
    // Test middle of range
    ColorResult result2 = run_color_test(128, 1000);
    EXPECT_EQ(result2.r, 128);
    EXPECT_EQ(result2.g, 0);
    EXPECT_EQ(result2.b, 0);
    
    // Test end of range
    ColorResult result3 = run_color_test(254, 1000);
    EXPECT_EQ(result3.r, 254);
    EXPECT_EQ(result3.g, 0);
    EXPECT_EQ(result3.b, 0);
}

// Test 4: Second color range (255 <= iterations < 510) - Red to Yellow
TEST_F(ColorMapperTestbench, ColorRange2_RedToYellow) {
    resetDUT();
    
    // Test beginning of range
    ColorResult result1 = run_color_test(255, 1000);
    EXPECT_EQ(result1.r, 255);
    EXPECT_EQ(result1.g, 0);
    EXPECT_EQ(result1.b, 0);
    
    // Test middle of range
    ColorResult result2 = run_color_test(383, 1000); // 255 + 128
    EXPECT_EQ(result2.r, 255);
    EXPECT_EQ(result2.g, 128);
    EXPECT_EQ(result2.b, 0);
    
    // Test end of range
    ColorResult result3 = run_color_test(509, 1000);
    EXPECT_EQ(result3.r, 255);
    EXPECT_EQ(result3.g, 254);
    EXPECT_EQ(result3.b, 0);
}

// Test 5: Third color range (510 <= iterations < 765) - Yellow to Cyan
TEST_F(ColorMapperTestbench, ColorRange3_YellowToCyan) {
    resetDUT();
    
    // Test beginning of range
    ColorResult result1 = run_color_test(510, 1000);
    EXPECT_EQ(result1.r, 255);
    EXPECT_EQ(result1.g, 255);
    EXPECT_EQ(result1.b, 0);
    
    // Test middle of range
    ColorResult result2 = run_color_test(638, 1000); // 510 + 128
    EXPECT_EQ(result2.r, 127); // 255 - 128
    EXPECT_EQ(result2.g, 255);
    EXPECT_EQ(result2.b, 128);
    
    // Test end of range
    ColorResult result3 = run_color_test(764, 1000);
    EXPECT_EQ(result3.r, 1);   // 255 - 254
    EXPECT_EQ(result3.g, 255);
    EXPECT_EQ(result3.b, 254);
}

// Test 6: Fourth color range (iterations >= 765) - Cyan to Blue
TEST_F(ColorMapperTestbench, ColorRange4_CyanToBlue) {
    resetDUT();
    
    // Test beginning of range
    ColorResult result1 = run_color_test(765, 1000);
    EXPECT_EQ(result1.r, 0);
    EXPECT_EQ(result1.g, 255);
    EXPECT_EQ(result1.b, 255);
    
    // Test middle of range
    ColorResult result2 = run_color_test(893, 1000); // 765 + 128
    EXPECT_EQ(result2.r, 0);
    EXPECT_EQ(result2.g, 127); // 255 - 128
    EXPECT_EQ(result2.b, 255);
    
    // Test high value (should wrap around due to 8-bit arithmetic)
    ColorResult result3 = run_color_test(1020, 1500); // 765 + 255
    EXPECT_EQ(result3.r, 0);
    EXPECT_EQ(result3.g, 0);   // 255 - 255
    EXPECT_EQ(result3.b, 255);
}

// Test 7: Boundary conditions between color ranges
TEST_F(ColorMapperTestbench, BoundaryConditions) {
    resetDUT();
    
    // Boundary between range 1 and 2
    ColorResult result1 = run_color_test(254, 1000);
    EXPECT_EQ(result1.r, 254);
    EXPECT_EQ(result1.g, 0);
    EXPECT_EQ(result1.b, 0);
    
    ColorResult result2 = run_color_test(255, 1000);
    EXPECT_EQ(result2.r, 255);
    EXPECT_EQ(result2.g, 0);
    EXPECT_EQ(result2.b, 0);
    
    // Boundary between range 2 and 3
    ColorResult result3 = run_color_test(509, 1000);
    EXPECT_EQ(result3.r, 255);
    EXPECT_EQ(result3.g, 254);
    EXPECT_EQ(result3.b, 0);
    
    ColorResult result4 = run_color_test(510, 1000);
    EXPECT_EQ(result4.r, 255);
    EXPECT_EQ(result4.g, 255);
    EXPECT_EQ(result4.b, 0);
    
    // Boundary between range 3 and 4
    ColorResult result5 = run_color_test(764, 1000);
    EXPECT_EQ(result5.r, 1);
    EXPECT_EQ(result5.g, 255);
    EXPECT_EQ(result5.b, 254);
    
    ColorResult result6 = run_color_test(765, 1000);
    EXPECT_EQ(result6.r, 0);
    EXPECT_EQ(result6.g, 255);
    EXPECT_EQ(result6.b, 255);
}

// Test 8: Edge case - iterations equals max_iter exactly
TEST_F(ColorMapperTestbench, ExactMaxIterations) {
    resetDUT();
    
    // Test various max_iter values
    ColorResult result1 = run_color_test(50, 50);
    EXPECT_EQ(result1.r, 0);
    EXPECT_EQ(result1.g, 0);
    EXPECT_EQ(result1.b, 0);
    
    ColorResult result2 = run_color_test(1000, 1000);
    EXPECT_EQ(result2.r, 0);
    EXPECT_EQ(result2.g, 0);
    EXPECT_EQ(result2.b, 0);
}

// Test 9: Multiple consecutive color mappings
TEST_F(ColorMapperTestbench, ConsecutiveColorMappings) {
    resetDUT();
    
    // Test a sequence of different iteration values
    ColorResult result1 = run_color_test(100, 1000);  // Red
    EXPECT_EQ(result1.r, 100);
    EXPECT_EQ(result1.g, 0);
    EXPECT_EQ(result1.b, 0);
    
    ColorResult result2 = run_color_test(300, 1000);  // Yellow
    EXPECT_EQ(result2.r, 255);
    EXPECT_EQ(result2.g, 45);  // 300 - 255
    EXPECT_EQ(result2.b, 0);
    
    ColorResult result3 = run_color_test(600, 1000);  // Cyan
    EXPECT_EQ(result3.r, 165);  // 255 - (600 - 510) = 255 - 90
    EXPECT_EQ(result3.g, 255);
    EXPECT_EQ(result3.b, 90);   // 600 - 510
    
    ColorResult result4 = run_color_test(800, 1000);  // Blue
    EXPECT_EQ(result4.r, 0);
    EXPECT_EQ(result4.g, 220);  // 255 - (800 - 765) = 255 - 35
    EXPECT_EQ(result4.b, 255);
    
    ColorResult result5 = run_color_test(1000, 1000); // Black (in set)
    EXPECT_EQ(result5.r, 0);
    EXPECT_EQ(result5.g, 0);
    EXPECT_EQ(result5.b, 0);
}

// Test 10: Verify pipeline behavior with rapid input changes
TEST_F(ColorMapperTestbench, RapidInputChanges) {
    resetDUT();
    
    // Set first input
    top->iterations_in = 100;
    top->max_iter = 1000;
    clockCycle();
    
    // Change input immediately
    top->iterations_in = 500;
    clockCycle();
    
    // Verify the output reflects the latest input
    // 500 is in range 2 (255 <= iterations < 510)
    // r = 255, g = iterations - 255 = 245, b = 0
    EXPECT_EQ(top->r, 255);
    EXPECT_EQ(top->g, 245);  // 500 - 255 = 245
    EXPECT_EQ(top->b, 0);
}
#include "base_testbench.h"
#include <cstdint>
#include <verilated_cov.h>
#include <gtest/gtest.h>
#include <cmath>

unsigned int ticks = 0;

// Fixed point conversion functions for Q8.24 format
int32_t double_to_fixed_point(double val) {
    return static_cast<int32_t>(val * (1LL << 24));
}

double fixed_point_to_double(int32_t val) {
    return static_cast<double>(val) / (1LL << 24);
}

class ScreenMapperTestbench : public BaseTestbench {
protected:
    void clockCycle() {
        // For combinational logic, just evaluate and dump
        top->eval();
        #ifndef __APPLE__
        tfp->dump(ticks);
        #endif
        ticks++;
    }

    void initializeInputs() override {
        top->x = 0;
        top->y = 0;
        top->pan_x = 0;
        top->pan_y = 0;
        top->zoom = 0;
    }

    void setInputs(uint16_t x_coord, uint16_t y_coord, double pan_x_val, double pan_y_val, uint8_t zoom_level) {
        top->x = x_coord;
        top->y = y_coord;
        top->pan_x = double_to_fixed_point(pan_x_val);
        top->pan_y = double_to_fixed_point(pan_y_val);
        top->zoom = zoom_level;
        top->eval(); // Evaluate combinational logic
        
        // Optional: dump for waveform viewing
        #ifndef __APPLE__
        tfp->dump(ticks);
        ticks++;
        #endif
    }

    void verifyOutput(double expected_c_re, double expected_c_im, double tolerance = 0.001) {
        double actual_c_re = fixed_point_to_double(static_cast<int32_t>(top->c_re));
        double actual_c_im = fixed_point_to_double(static_cast<int32_t>(top->c_im));
        
        // Debug output for troubleshooting
        if (std::abs(actual_c_re - expected_c_re) > tolerance || std::abs(actual_c_im - expected_c_im) > tolerance) {
            std::cout << "MISMATCH - Expected: c_re=" << expected_c_re << ", c_im=" << expected_c_im << std::endl;
            std::cout << "MISMATCH - Actual: c_re=" << actual_c_re << ", c_im=" << actual_c_im << std::endl;
            std::cout << "MISMATCH - Raw: c_re=0x" << std::hex << top->c_re << ", c_im=0x" << top->c_im << std::dec << std::endl;
        }
        
        EXPECT_NEAR(actual_c_re, expected_c_re, tolerance) 
            << "c_re mismatch: expected " << expected_c_re << ", got " << actual_c_re;
        EXPECT_NEAR(actual_c_im, expected_c_im, tolerance)
            << "c_im mismatch: expected " << expected_c_im << ", got " << actual_c_im;
    }

    // Updated calculation for 640x480 resolution (center at 320, 240)
    std::pair<double, double> calculateExpected(uint16_t x, uint16_t y, double pan_x, double pan_y, uint8_t zoom) {
        // Step 1: Center the coordinates (320, 240 for 640x480)
        int32_t x_centered = static_cast<int32_t>(x) - 320;
        int32_t y_centered = static_cast<int32_t>(y) - 240;
        
        // Step 2: Convert to fixed point (Q8.24 format)
        int64_t x_shifted = static_cast<int64_t>(x_centered) << 24;
        int64_t y_shifted = static_cast<int64_t>(y_centered) << 24;
        
        // Step 3: Apply zoom (arithmetic right shift)
        // Limit zoom to match Verilog's 5-bit limitation
        uint8_t zoom_limited = (zoom > 24) ? 24 : zoom;
        int64_t x_zoomed = x_shifted >> zoom_limited;
        int64_t y_zoomed = y_shifted >> zoom_limited;
        
        // Step 4: Scale down by 16 (4-bit right shift) and add pan
        int64_t c_re_fixed = (x_zoomed >> 4) + double_to_fixed_point(pan_x);
        int64_t c_im_fixed = (y_zoomed >> 4) + double_to_fixed_point(pan_y);
        
        double c_re = fixed_point_to_double(static_cast<int32_t>(c_re_fixed));
        double c_im = fixed_point_to_double(static_cast<int32_t>(c_im_fixed));
        
        return std::make_pair(c_re, c_im);
    }
};

// Comprehensive debug test
TEST_F(ScreenMapperTestbench, ComprehensiveDebugTest) {
    std::cout << "\n=== COMPREHENSIVE DEBUG TEST (640x480) ===" << std::endl;
    
    // Test 1: Check if module is responding at all
    std::cout << "\n--- Test 1: Basic module response ---" << std::endl;
    top->x = 320;  // Center X for 640x480
    top->y = 240;  // Center Y for 640x480
    top->pan_x = 0;
    top->pan_y = 0;
    top->zoom = 0;
    top->eval();
    
    std::cout << "Inputs: x=" << top->x << ", y=" << top->y 
              << ", pan_x=0x" << std::hex << top->pan_x 
              << ", pan_y=0x" << top->pan_y 
              << ", zoom=" << std::dec << (int)top->zoom << std::endl;
    std::cout << "Outputs: c_re=0x" << std::hex << top->c_re 
              << ", c_im=0x" << top->c_im << std::dec << std::endl;
    
    // Test 2: Try simple non-center coordinates
    std::cout << "\n--- Test 2: Non-center coordinates ---" << std::endl;
    top->x = 321;  // One pixel right of center
    top->y = 241;  // One pixel down from center
    top->pan_x = 0;
    top->pan_y = 0;
    top->zoom = 0;
    top->eval();
    
    std::cout << "Inputs: x=" << top->x << ", y=" << top->y << std::endl;
    std::cout << "Outputs: c_re=0x" << std::hex << top->c_re 
              << ", c_im=0x" << top->c_im << std::dec << std::endl;
    
    // Test 3: Check input/output sizes
    std::cout << "\n--- Test 3: Input/Output sizes ---" << std::endl;
    std::cout << "sizeof(top->x) = " << sizeof(top->x) << " bytes" << std::endl;
    std::cout << "sizeof(top->y) = " << sizeof(top->y) << " bytes" << std::endl;
    std::cout << "sizeof(top->pan_x) = " << sizeof(top->pan_x) << " bytes" << std::endl;
    std::cout << "sizeof(top->zoom) = " << sizeof(top->zoom) << " bytes" << std::endl;
    std::cout << "sizeof(top->c_re) = " << sizeof(top->c_re) << " bytes" << std::endl;
    
    std::cout << "=== END COMPREHENSIVE DEBUG ===" << std::endl;
    
    EXPECT_TRUE(true); // This test is for debugging only
}

// Test 1: Center point (320, 240) with no pan or zoom
TEST_F(ScreenMapperTestbench, CenterPoint_NoPanNoZoom) {
    setInputs(320, 240, 0.0, 0.0, 0);
    
    // Debug the actual output first
    std::cout << "Center point test - Raw outputs: c_re=0x" << std::hex << top->c_re 
              << ", c_im=0x" << top->c_im << std::dec << std::endl;
    std::cout << "Center point test - Converted: c_re=" << fixed_point_to_double(static_cast<int32_t>(top->c_re))
              << ", c_im=" << fixed_point_to_double(static_cast<int32_t>(top->c_im)) << std::endl;
    
    // At center with no pan/zoom, both c_re and c_im should be 0
    verifyOutput(0.0, 0.0);
}

// Test 2: Simple off-center test
TEST_F(ScreenMapperTestbench, SimpleOffCenter) {
    // Test one pixel to the right of center
    setInputs(321, 240, 0.0, 0.0, 0);
    auto expected = calculateExpected(321, 240, 0.0, 0.0, 0);
    std::cout << "Off-center test - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}

// Test 3: Test panning effect
TEST_F(ScreenMapperTestbench, SimplePanning) {
    const double pan_x_val = 0.5;
    const double pan_y_val = -0.3;
    
    // Center point with panning - should just return the pan values
    setInputs(320, 240, pan_x_val, pan_y_val, 0);
    verifyOutput(pan_x_val, pan_y_val);
}

// Test 4: Test corner points for 640x480 resolution
TEST_F(ScreenMapperTestbench, CornerPoints_NoPanNoZoom_640x480) {
    // Top-left corner (0, 0)
    setInputs(0, 0, 0.0, 0.0, 0);
    auto expected = calculateExpected(0, 0, 0.0, 0.0, 0);
    std::cout << "Top-left corner - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
    
    // Top-right corner (639, 0) - for 640x480, max X is 639
    setInputs(639, 0, 0.0, 0.0, 0);
    expected = calculateExpected(639, 0, 0.0, 0.0, 0);
    std::cout << "Top-right corner - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
    
    // Bottom-left corner (0, 479) - for 640x480, max Y is 479
    setInputs(0, 479, 0.0, 0.0, 0);
    expected = calculateExpected(0, 479, 0.0, 0.0, 0);
    std::cout << "Bottom-left corner - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
    
    // Bottom-right corner (639, 479)
    setInputs(639, 479, 0.0, 0.0, 0);
    expected = calculateExpected(639, 479, 0.0, 0.0, 0);
    std::cout << "Bottom-right corner - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}

// Test 5: Test zoom effect
TEST_F(ScreenMapperTestbench, SimpleZoom) {
    // Test zoom on off-center point
    setInputs(400, 300, 0.0, 0.0, 2);
    auto expected = calculateExpected(400, 300, 0.0, 0.0, 2);
    std::cout << "Zoom test - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}

// Test 6: Test zoom limiting (should handle zoom > 24)
TEST_F(ScreenMapperTestbench, ZoomLimiting) {
    // Test with zoom value > 24 to verify limiting works
    setInputs(400, 300, 0.0, 0.0, 30);  // Should be limited to 24
    auto expected = calculateExpected(400, 300, 0.0, 0.0, 30);  // calculateExpected handles limiting
    std::cout << "Zoom limiting test - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}

// Test 7: Test combined pan and zoom
TEST_F(ScreenMapperTestbench, CombinedPanAndZoom) {
    const double pan_x_val = -1.2;
    const double pan_y_val = 0.8;
    const uint8_t zoom_level = 3;
    
    setInputs(500, 350, pan_x_val, pan_y_val, zoom_level);
    auto expected = calculateExpected(500, 350, pan_x_val, pan_y_val, zoom_level);
    std::cout << "Combined pan/zoom test - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}

// Test 8: Test edge cases with maximum input values
TEST_F(ScreenMapperTestbench, MaxInputValues) {
    // Test with maximum 10-bit input values (1023, 1023)
    // Note: This is beyond the 640x480 screen, but tests the 10-bit input range
    setInputs(1023, 1023, 0.0, 0.0, 0);
    auto expected = calculateExpected(1023, 1023, 0.0, 0.0, 0);
    std::cout << "Max input values test - Expected: c_re=" << expected.first << ", c_im=" << expected.second << std::endl;
    verifyOutput(expected.first, expected.second);
}
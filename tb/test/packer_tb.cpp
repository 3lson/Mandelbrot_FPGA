#include "base_testbench.h"
#include <cstdint>
#include <verilated_cov.h>
#include <gtest/gtest.h>
#include <vector>
#include <queue>

unsigned int ticks = 0;

class PackerTestbench : public BaseTestbench {
protected:
    void clockCycle() {
        top->aclk = 0;
        top->eval();

        top->aclk = 1;
        top->eval();
        ticks++;
    }

    void initializeInputs() override {
        top->aresetn = 0; // Start in reset
        top->r = 0;
        top->g = 0;
        top->b = 0;
        top->eol = 0;
        top->valid = 0;
        top->sof = 0;
        top->out_stream_tready = 1; // Default to ready
    }

    void resetDUT() {
        // Reset sequence - hold reset for a few cycles
        top->aresetn = 0;
        clockCycle();
        clockCycle();
        top->aresetn = 1; // Release reset
        clockCycle();
        
        // Verify initial conditions (STATE_IDLE)
        ASSERT_EQ(top->in_stream_ready, 1);
        ASSERT_EQ(top->out_stream_tvalid, 0);
        ASSERT_EQ(top->out_stream_tlast, 0);
        ASSERT_EQ(top->out_stream_tuser, 0);
        ASSERT_EQ(top->out_stream_tkeep, 0xF);
    }

    void sendPixel(uint8_t r, uint8_t g, uint8_t b, bool sof = false, bool eol = false) {
        // Wait for ready if not ready
        int timeout = 100;
        while (!top->in_stream_ready && timeout > 0) {
            clockCycle();
            timeout--;
        }
        ASSERT_GT(timeout, 0) << "Timeout waiting for input ready";

        // Set up the inputs
        top->r = r;
        top->g = g;
        top->b = b;
        top->sof = sof ? 1 : 0;
        top->eol = eol ? 1 : 0;
        top->valid = 1;
        
        // Clock to process the input (moves to STATE_SEND)
        clockCycle();
        
        // Clear inputs immediately after clocking
        top->valid = 0;
        top->sof = 0;
        top->eol = 0;
    }

    void clearInputs() {
        top->valid = 0;
        top->sof = 0;
        top->eol = 0;
        top->r = 0;
        top->g = 0;
        top->b = 0;
    }

    void waitForOutput() {
        int timeout = 100;
        while (!top->out_stream_tvalid && timeout > 0) {
            clockCycle();
            timeout--;
        }
        ASSERT_GT(timeout, 0) << "Timeout waiting for output";
    }

    void checkOutput(uint32_t expected_tdata, bool expected_tlast, bool expected_tuser) {
        EXPECT_EQ(top->out_stream_tdata, expected_tdata) 
            << "Expected tdata: 0x" << std::hex << expected_tdata 
            << ", Got: 0x" << std::hex << top->out_stream_tdata;
        EXPECT_EQ(top->out_stream_tlast, expected_tlast ? 1 : 0);
        EXPECT_EQ(top->out_stream_tuser, expected_tuser ? 1 : 0);
        EXPECT_EQ(top->out_stream_tkeep, 0xF);
        EXPECT_EQ(top->out_stream_tvalid, 1);
    }

    void acceptOutput() {
        if (top->out_stream_tvalid) {
            top->out_stream_tready = 1;
            clockCycle();
        }
    }

    uint32_t formatPixel(uint8_t r, uint8_t g, uint8_t b) {
        // Format: 0x00RRGGBB
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(b);
    }

    // Debug helper to print current state
    void printState() {
        printf("State: valid=%d, ready=%d, tvalid=%d, tdata=0x%08x, tuser=%d, tlast=%d\n",
               top->valid, top->in_stream_ready, top->out_stream_tvalid, 
               top->out_stream_tdata, top->out_stream_tuser, top->out_stream_tlast);
    }
};

// Test 1: Basic reset and initialization test
TEST_F(PackerTestbench, ResetTest) {
    resetDUT();
    
    // After reset, should be in STATE_IDLE with no valid output
    EXPECT_EQ(top->out_stream_tvalid, 0);
    EXPECT_EQ(top->in_stream_ready, 1);
    EXPECT_EQ(top->out_stream_tuser, 0);
    EXPECT_EQ(top->out_stream_tlast, 0);
    
    printf("Reset test passed - Initial state correct\n");
}

// Test 2: Single pixel with SOF
TEST_F(PackerTestbench, SinglePixelSOFTest) {
    resetDUT();
    
    printf("=== Single Pixel SOF Test ===\n");
    
    // Send a single pixel with SOF
    printf("Sending pixel with SOF: R=0x11, G=0x22, B=0x33\n");
    sendPixel(0x11, 0x22, 0x33, true, false);
    
    // After sending, should be in STATE_SEND with valid output
    printf("After pixel: tvalid=%d, tdata=0x%08x, tuser=%d, tlast=%d\n", 
           top->out_stream_tvalid, top->out_stream_tdata, top->out_stream_tuser, top->out_stream_tlast);
    
    EXPECT_EQ(top->out_stream_tvalid, 1) << "Should have valid output in STATE_SEND";
    
    uint32_t expected_data = formatPixel(0x11, 0x22, 0x33);
    checkOutput(expected_data, false, true); // SOF=true, EOL=false
    
    // Accept the output
    acceptOutput();
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    EXPECT_EQ(top->in_stream_ready, 1) << "Should be ready for next input";
    
    printf("Single pixel SOF test passed\n");
}

// Test 3: Single pixel with EOL
TEST_F(PackerTestbench, SinglePixelEOLTest) {
    resetDUT();
    
    printf("=== Single Pixel EOL Test ===\n");
    
    // Send a single pixel with EOL
    printf("Sending pixel with EOL: R=0x44, G=0x55, B=0x66\n");
    sendPixel(0x44, 0x55, 0x66, false, true);
    
    // Should have valid output with tlast set
    EXPECT_EQ(top->out_stream_tvalid, 1) << "Should have valid output";
    
    uint32_t expected_data = formatPixel(0x44, 0x55, 0x66);
    checkOutput(expected_data, true, false); // SOF=false, EOL=true
    
    acceptOutput();
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    
    printf("Single pixel EOL test passed\n");
}

// Test 4: Single pixel with both SOF and EOL
TEST_F(PackerTestbench, SinglePixelSOFEOLTest) {
    resetDUT();
    
    printf("=== Single Pixel SOF+EOL Test ===\n");
    
    // Send a single pixel with both SOF and EOL
    printf("Sending pixel with SOF+EOL: R=0x77, G=0x88, B=0x99\n");
    sendPixel(0x77, 0x88, 0x99, true, true);
    
    // Should have valid output with both tuser and tlast set
    EXPECT_EQ(top->out_stream_tvalid, 1) << "Should have valid output";
    
    uint32_t expected_data = formatPixel(0x77, 0x88, 0x99);
    checkOutput(expected_data, true, true); // SOF=true, EOL=true
    
    acceptOutput();
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    
    printf("Single pixel SOF+EOL test passed\n");
}

// Test 5: Multiple pixels in sequence
TEST_F(PackerTestbench, MultiplePixelsTest) {
    resetDUT();
    
    printf("=== Multiple Pixels Test ===\n");
    
    // Pixel 1: SOF
    printf("Sending pixel 1 (SOF): R=0x01, G=0x02, B=0x03\n");
    sendPixel(0x01, 0x02, 0x03, true, false);
    
    uint32_t expected_data1 = formatPixel(0x01, 0x02, 0x03);
    checkOutput(expected_data1, false, true); // SOF=true, EOL=false
    acceptOutput();
    
    // Pixel 2: Middle pixel
    printf("Sending pixel 2: R=0x04, G=0x05, B=0x06\n");
    sendPixel(0x04, 0x05, 0x06, false, false);
    
    uint32_t expected_data2 = formatPixel(0x04, 0x05, 0x06);
    checkOutput(expected_data2, false, false); // SOF=false, EOL=false
    acceptOutput();
    
    // Pixel 3: EOL
    printf("Sending pixel 3 (EOL): R=0x07, G=0x08, B=0x09\n");
    sendPixel(0x07, 0x08, 0x09, false, true);
    
    uint32_t expected_data3 = formatPixel(0x07, 0x08, 0x09);
    checkOutput(expected_data3, true, false); // SOF=false, EOL=true
    acceptOutput();
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    
    printf("Multiple pixels test passed\n");
}

// Test 6: Backpressure handling
TEST_F(PackerTestbench, BackpressureTest) {
    resetDUT();
    
    printf("=== Backpressure Test ===\n");
    
    // Send a pixel
    printf("Sending pixel: R=0xAA, G=0xBB, B=0xCC\n");
    sendPixel(0xAA, 0xBB, 0xCC, true, false);
    
    // Should have valid output
    EXPECT_EQ(top->out_stream_tvalid, 1) << "Should have valid output";
    
    // Apply backpressure
    printf("Applying backpressure (tready=0)\n");
    top->out_stream_tready = 0;
    clockCycle();
    
    // Should still have valid output but input should not be ready
    EXPECT_EQ(top->out_stream_tvalid, 1) << "Output should still be valid";
    EXPECT_EQ(top->in_stream_ready, 0) << "Input should not be ready during backpressure";
    
    // Release backpressure
    printf("Releasing backpressure\n");
    top->out_stream_tready = 1;
    clockCycle();
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    EXPECT_EQ(top->in_stream_ready, 1) << "Input should be ready again";
    
    printf("Backpressure test passed\n");
}

// Test 7: Verify data format
TEST_F(PackerTestbench, DataFormatTest) {
    resetDUT();
    
    printf("=== Data Format Test ===\n");
    
    // Test specific RGB values to verify data format
    uint8_t test_r = 0xAB;
    uint8_t test_g = 0xCD;
    uint8_t test_b = 0xEF;
    
    printf("Sending pixel: R=0x%02X, G=0x%02X, B=0x%02X\n", test_r, test_g, test_b);
    sendPixel(test_r, test_g, test_b, false, false);
    
    // Expected format: 0x00RRGGBB
    uint32_t expected = (static_cast<uint32_t>(test_r) << 16) |
                       (static_cast<uint32_t>(test_g) << 8) |
                       static_cast<uint32_t>(test_b);
    
    printf("Expected: 0x%08X, Got: 0x%08X\n", expected, top->out_stream_tdata);
    
    EXPECT_EQ(top->out_stream_tdata, expected) << "Data format should be 0x00RRGGBB";
    EXPECT_EQ(top->out_stream_tkeep, 0xF) << "All bytes should be kept";
    
    acceptOutput();
    
    printf("Data format test passed\n");
}

// Test 8: Rapid fire pixels
TEST_F(PackerTestbench, RapidFireTest) {
    resetDUT();
    
    printf("=== Rapid Fire Test ===\n");
    
    // Send multiple pixels rapidly
    for (int i = 0; i < 5; i++) {
        bool is_sof = (i == 0);
        bool is_eol = (i == 4);
        uint8_t val = 0x10 + i;
        
        printf("Sending pixel %d: R=0x%02X, G=0x%02X, B=0x%02X, SOF=%d, EOL=%d\n", 
               i, val, val+1, val+2, is_sof, is_eol);
        
        sendPixel(val, val+1, val+2, is_sof, is_eol);
        
        // Verify output
        uint32_t expected = formatPixel(val, val+1, val+2);
        checkOutput(expected, is_eol, is_sof);
        
        acceptOutput();
    }
    
    // Should be back in STATE_IDLE
    EXPECT_EQ(top->out_stream_tvalid, 0) << "Should be back in STATE_IDLE";
    
    printf("Rapid fire test passed\n");
}
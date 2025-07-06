#include "base_testbench.h" // Assuming .hh extension from common practice
#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <verilated_cov.h>

// Global tick counter
unsigned int ticks = 0;

// Helper struct to hold captured AXI-Stream pixel data
struct PixelData {
    uint32_t data; // tdata (contains RGB)
    bool last;     // tlast (end of line)
    bool user;     // tuser (start of frame)
};

class PixelGeneratorTestbench : public BaseTestbench {
protected:
    // Toggles both clocks for simplicity. In a real complex system,
    // these might be asynchronous, but for this testbench, a shared
    // clock is sufficient and standard practice.
    void clockCycle() {
        top->s_axi_lite_aclk = 0;
        top->out_stream_aclk = 0;
        top->eval();
        #ifndef __APPLE__
        tfp->dump(2 * ticks);
        #endif

        top->s_axi_lite_aclk = 1;
        top->out_stream_aclk = 1;
        top->eval();
        #ifndef __APPLE__
        tfp->dump(2 * ticks + 1);
        #endif
        ticks++;
    }

    // Set all inputs to a known, idle state
    void initializeInputs() override {
        // Active-low resets are asserted (0) initially
        top->axi_resetn = 0;
        top->periph_resetn = 0;

        // AXI-Stream consumer is not ready initially
        top->out_stream_tready = 0;

        // AXI-Lite master interfaces are idle
        top->s_axi_lite_arvalid = 0;
        top->s_axi_lite_awvalid = 0;
        top->s_axi_lite_wvalid = 0;
        top->s_axi_lite_bready = 0;
        top->s_axi_lite_rready = 0;
    }

    // Apply and release resets to bring DUT to an operational state
    void resetDUT() {
        initializeInputs();
        top->axi_resetn = 0;
        top->periph_resetn = 0;
        clockCycle();
        clockCycle();
        top->axi_resetn = 1;
        top->periph_resetn = 1;
        clockCycle();
        // The DUT is now out of reset and should start its internal FSM
    }

    // Helper function to perform a complete AXI-Lite write transaction
    void axi_lite_write(uint32_t addr, uint32_t data) {
        top->s_axi_lite_awaddr = addr;
        top->s_axi_lite_awvalid = 1;
        top->s_axi_lite_wdata = data;
        top->s_axi_lite_wvalid = 1;

        // Wait until the DUT is ready for both address and data
        while (!(top->s_axi_lite_awready && top->s_axi_lite_wready)) {
            clockCycle();
        }

        clockCycle();
        top->s_axi_lite_awvalid = 0;
        top->s_axi_lite_wvalid = 0;

        // Wait for the write response
        top->s_axi_lite_bready = 1;
        while (!top->s_axi_lite_bvalid) {
            clockCycle();
        }
        
        // Expect a successful response (AXI_OK)
        EXPECT_EQ(top->s_axi_lite_bresp, 0);
        clockCycle();
        top->s_axi_lite_bready = 0;
    }

    // Helper function to perform a complete AXI-Lite read transaction
    uint32_t axi_lite_read(uint32_t addr) {
        top->s_axi_lite_araddr = addr;
        top->s_axi_lite_arvalid = 1;

        while (!top->s_axi_lite_arready) {
            clockCycle();
        }

        clockCycle();
        top->s_axi_lite_arvalid = 0;

        // Wait for the DUT to provide valid read data
        top->s_axi_lite_rready = 1;
        while (!top->s_axi_lite_rvalid) {
            clockCycle();
        }

        uint32_t read_data = top->s_axi_lite_rdata;
        EXPECT_EQ(top->s_axi_lite_rresp, 0); // Expect AXI_OK
        clockCycle();
        top->s_axi_lite_rready = 0;
        
        return read_data;
    }

    // Helper function to capture a stream of pixels.
    // Includes a timeout to detect if the DUT stops producing pixels.
    std::vector<PixelData> read_frame(int width, int height) {
        std::vector<PixelData> pixels;
        pixels.reserve(width * height);
        
        // Generous timeout: 50 cycles per pixel should be more than enough
        // given that max_iter is usually ~100.
        long timeout_cycles = (long)width * height * 50;

        // Signal that the consumer is always ready to accept data
        top->out_stream_tready = 1;

        while (pixels.size() < width * height && timeout_cycles > 0) {
            // AXI Stream handshake: transaction occurs when tvalid and tready are both high
            if (top->out_stream_tvalid && top->out_stream_tready) {
                PixelData p;
                p.data = top->out_stream_tdata;
                p.user = top->out_stream_tuser;
                p.last = top->out_stream_tlast;
                pixels.push_back(p);
            }
            clockCycle();
            timeout_cycles--;
        }

        // If the timeout was hit, it's a critical failure.
        EXPECT_GT(timeout_cycles, 0) << "Timeout! DUT stopped sending pixels. Received "
                                     << pixels.size() << " of " << width * height << " pixels.";
        
        return pixels;
    }
};

// Test 1: Verify AXI-Lite register read/write functionality
TEST_F(PixelGeneratorTestbench, RegisterReadWrite) {
    resetDUT();
    
    // Write arbitrary values to the registers
    axi_lite_write(0x00, 250);          // max_iter
    axi_lite_write(0x04, 0x11223344);   // pan_x
    axi_lite_write(0x08, 0xAABBCCDD);   // pan_y
    axi_lite_write(0x0C, 0x02000000);   // zoom (2.0 in Q1.31 would be 0x20000000)
    
    // Read them back and verify
    EXPECT_EQ(axi_lite_read(0x00), 250);
    EXPECT_EQ(axi_lite_read(0x04), 0x11223344);
    EXPECT_EQ(axi_lite_read(0x08), 0xAABBCCDD);
    EXPECT_EQ(axi_lite_read(0x0C), 0x02000000);
}

// Test 2: Verify that the first pixel of a frame has the TUSER (SOF) signal asserted.
TEST_F(PixelGeneratorTestbench, StartOfFrameSignal) {
    resetDUT();
    // Use default parameters
    axi_lite_write(0x00, 50); // Use a low max_iter to speed up simulation

    // We only need to check the very first pixel
    top->out_stream_tready = 1;
    while (!top->out_stream_tvalid) {
        clockCycle();
    }

    // The first valid pixel MUST have tuser asserted
    EXPECT_TRUE(top->out_stream_tuser) << "The first pixel of the frame did not have TUSER (SOF) asserted.";

    // The next pixel should NOT have tuser asserted
    clockCycle(); // Move to the next pixel
    while (!top->out_stream_tvalid) {
        clockCycle();
    }
    EXPECT_FALSE(top->out_stream_tuser) << "TUSER was asserted on the second pixel.";
}

// Test 3: Verify that the TLAST (EOL) signal is asserted correctly at the end of a line.
TEST_F(PixelGeneratorTestbench, EndOfLineSignal) {
    resetDUT();
    axi_lite_write(0x00, 50); // Low max_iter for speed
    
    const int WIDTH = 640;
    // Capture just over one line of pixels
    auto pixels = read_frame(WIDTH + 2, 1); 

    // This check is only valid if we received enough pixels
    ASSERT_GE(pixels.size(), WIDTH + 1);

    // The pixel at x = width - 2 should NOT have tlast
    EXPECT_FALSE(pixels[WIDTH - 2].last) << "TLAST was asserted prematurely on the second to last pixel of the line.";

    // The pixel at x = width - 1 SHOULD have tlast
    EXPECT_TRUE(pixels[WIDTH - 1].last) << "TLAST was not asserted on the last pixel of the line.";
    
    // The first pixel of the next line (at index WIDTH) should NOT have tlast
    EXPECT_FALSE(pixels[WIDTH].last) << "TLAST was asserted on the first pixel of the second line.";
}

// Test 4: Run a full frame generation and check final properties.
// This test will FAIL if the FSM doesn't correctly reset at the end of the frame,
// as the read_frame() function will time out.
TEST_F(PixelGeneratorTestbench, FullFrameGeneration) {
    resetDUT();
    const int WIDTH = 640;
    const int HEIGHT = 480;

    // Use default parameters (pan=0, zoom=1.0) and a low max_iter
    axi_lite_write(0x00, 30);
    
    auto frame = read_frame(WIDTH, HEIGHT);

    // If the test timed out, the size will be wrong and the test will fail here.
    ASSERT_EQ(frame.size(), WIDTH * HEIGHT) << "Did not receive the complete frame.";
    
    // Verify properties of the complete frame
    EXPECT_TRUE(frame[0].user) << "First pixel's TUSER was not set.";
    EXPECT_TRUE(frame.back().last) << "Final pixel's TLAST was not set.";

    // With pan=0, zoom=1, max_iter=30, the center of the screen is in the set (black)
    // Data format is {8'h00, r, g, b}
    int center_pixel_index = (HEIGHT / 2) * WIDTH + (WIDTH / 2);
    EXPECT_EQ(frame[center_pixel_index].data, 0x00000000) << "Center pixel was not black.";
}
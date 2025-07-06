module mandelbrot_calculator (
    input                   clk,
    input                   rst,

    // Control signals
    input                   start,
    output logic            ready,

    // Parameters from AXI-Lite
    input      [31:0]       c_re,
    input      [31:0]       c_im,
    input      [31:0]       max_iter,

    // Output
    output logic [31:0]     iterations
);

    // Internal registers
    reg [31:0] z_re, z_im;
    reg [31:0] iter_count;

    // Pipeline registers for multiplication
    reg [63:0] z_re_sq, z_im_sq, z_2ab;
    reg [31:0] z_re_sq_reg, z_im_sq_reg, z_2ab_reg;
    reg        cook;       // Master signal: calculation is in progress
    reg        cook_state; // 0 = Calculate z^2; 1 = Calculate next z
    
    localparam [31:0] ESCAPE_THRESHOLD = 32'h40000000;

    always_comb begin
        z_re_sq = $signed(z_re) * $signed(z_re);
        z_im_sq = $signed(z_im) * $signed(z_im);
        z_2ab   = ($signed(z_re) * $signed(z_im)) << 1;
    end

    // Pipeline registers - Stage 1
    always_ff @(posedge clk) begin
        if (rst) begin
            z_re_sq_reg <= 0;
            z_im_sq_reg <= 0;
            z_2ab_reg   <= 0;
        end else if (cook && !cook_state) begin
            // Latch pipeline values when entering calculation state
            z_re_sq_reg <= z_re_sq[59:28];
            z_im_sq_reg <= z_im_sq[59:28];
            z_2ab_reg   <= z_2ab[59:28];
        end
    end

    // Main calculation logic - Stage 2
    always_ff @(posedge clk) begin
        if (rst) begin
            z_re <= 0;
            z_im <= 0;
            iter_count <= 0;
            cook <= 0;
            cook_state <= 0;
            ready <= 1;
        end else begin
            if (start && ready) begin
                // Start a new calculation with z0 = 0
                z_re <= 0;
                z_im <= 0;
                iter_count <= 0;
                cook <= 1;
                cook_state <= 0; // Start with calculating z^2
                ready <= 0;
            end else if (cook) begin
                if (!cook_state) begin
                    // PHASE 0: Check if we should terminate before calculating next z
                    if (iter_count >= max_iter) begin
                        // Max iterations reached
                        cook <= 0;
                        ready <= 1;
                    end else begin
                        // Check escape condition using current z
                        // For the very first iteration (iter_count == 0), z is still 0
                        if (iter_count > 0 && (z_re_sq_reg + z_im_sq_reg >= ESCAPE_THRESHOLD)) begin
                            // Escaped
                            cook <= 0;
                            ready <= 1;
                        end else begin
                            // Continue to calculate next z
                            cook_state <= 1;
                        end
                    end
                end else begin
                    // PHASE 1: Calculate next z using pipeline registers
                    // z_new = z^2 + c
                    z_re <= $signed(z_re_sq_reg) - $signed(z_im_sq_reg) + $signed(c_re);
                    z_im <= $signed(z_2ab_reg) + $signed(c_im);
                    iter_count <= iter_count + 1;
                    cook_state <= 0; // Go back to phase 0 for next iteration
                end
            end
        end
    end

    assign iterations = iter_count;

endmodule

module color_mapper (
    input           clk,
    input [31:0]    iterations_in,
    input [31:0]    max_iter,

    output logic [7:0] r,
    output logic [7:0] g,
    output logic [7:0] b
);
    reg [7:0] r_reg, g_reg, b_reg;

    // We can use a smaller stretch factor now that our cycle is more efficient.
    localparam STRETCH_FACTOR = 4;

    // This wire still creates the fast-growing iteration count.
    wire [31:0] stretched_iters = iterations_in * STRETCH_FACTOR;

    always_ff @(posedge clk) begin
        if (iterations_in >= max_iter) begin
            // Correctly set points inside the set to black.
            r_reg <= 8'h00;
            g_reg <= 8'h00;
            b_reg <= 8'h00;
        end else begin
            // Use the top 2 bits of a 10-bit counter as a mode selector.
            // This replaces the non-synthesizable `case...%...` statement.
            case (stretched_iters[9:8])
                // Mode 0: Corresponds to original range 0-255. Ramp up Red.
                2'b00: begin
                    r_reg <= stretched_iters[7:0]; // Use the lower 8 bits for the ramp
                    g_reg <= 0;
                    b_reg <= 0;
                end
                // Mode 1: Corresponds to original range 256-511. Ramp up Green.
                2'b01: begin
                    r_reg <= 255;
                    g_reg <= stretched_iters[7:0];
                    b_reg <= 0;
                end
                // Mode 2: Corresponds to original range 512-767. Ramp up Blue, fade out Red.
                2'b10: begin
                    r_reg <= ~stretched_iters[7:0]; // Invert to fade from 255 down to 0
                    g_reg <= 255;
                    b_reg <= stretched_iters[7:0];
                end
                // Mode 3: Corresponds to range 768-1023. Fade out Green.
                2'b11: begin
                    r_reg <= 0;
                    g_reg <= ~stretched_iters[7:0];
                    b_reg <= 255;
                end
                // A default is required for a case statement to be synthesizable
                default: begin
                    r_reg <= 0;
                    g_reg <= 0;
                    b_reg <= 0;
                end
            endcase
        end
    end

    assign r = r_reg;
    assign g = g_reg;
    assign b = b_reg;

endmodule
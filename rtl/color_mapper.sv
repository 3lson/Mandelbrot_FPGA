module color_mapper (
    input           clk,
    input [31:0]    iterations_in,
    input [31:0]    max_iter,

    output logic [7:0] r,
    output logic [7:0] g,
    output logic [7:0] b
);
    reg [7:0] r_reg, g_reg, b_reg;

    always_ff @(posedge clk) begin
        if (iterations_in >= max_iter) begin
            r_reg <= 8'h00;
            g_reg <= 8'h00;
            b_reg <= 8'h00;
        end else begin
            if (iterations_in < 255) begin
                r_reg <= iterations_in[7:0];
                g_reg <= 0;
                b_reg <= 0;
            end else if (iterations_in < 510) begin
                r_reg <= 255;
                g_reg <= iterations_in[7:0] - 255;
                b_reg <= 0;
            end else if (iterations_in < 765) begin
                r_reg <= 255 - (iterations_in[7:0] - 510);
                g_reg <= 255;
                b_reg <= iterations_in[7:0] - 510;
            end else begin
                r_reg <= 0;
                g_reg <= 255 - (iterations_in[7:0] - 765);
                b_reg <= 255;
            end
        end
    end

    assign r = r_reg;
    assign g = g_reg;
    assign b = b_reg;

endmodule

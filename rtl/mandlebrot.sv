module mandelbrot #(
    parameter DATA_WIDTH = 32,      // Total bits for our fixed-point numbers (Q4.28)
    parameter FRAC_WIDTH = 28       // Fractional bits
)(
    input  logic clk,
    input  logic rst,
    input  logic start,            

    input  logic signed [DATA_WIDTH-1:0] c_real_in,
    input  logic signed [DATA_WIDTH-1:0] c_imag_in,
    input  logic [15:0] max_iterations_in,

    output logic [15:0] iteration_count_out,
    output logic done
);

    typedef enum logic [1:0] { S_IDLE, S_COMPUTE, S_DONE } state_t;
    state_t state, next_state;

    logic signed [DATA_WIDTH-1:0] z_real, z_imag;
    logic [15:0] current_iter;

    logic signed [DATA_WIDTH-1:0] z_real_next, z_imag_next;
    
    logic signed [DATA_WIDTH*2-1:0] zr_sq, zi_sq, zr_zi; 

    localparam logic signed [DATA_WIDTH*2-1:0] ESCAPE_RADIUS_SQ = 4'd4 <<< (FRAC_WIDTH * 2);

    always_ff @(posedge clk) begin
        if (rst) begin
            state <= S_IDLE;
        end else begin
            state <= next_state;
        end
    end

    always_comb begin
        next_state = state;
        done = 1'b0;

        case(state)
            S_IDLE: begin
                if (start) begin
                    next_state = S_COMPUTE;
                end
            end
            
            S_COMPUTE: begin
                if ((zr_sq + zi_sq) >= ESCAPE_RADIUS_SQ) begin
                    next_state = S_DONE;
                end else if (current_iter >= max_iterations_in) begin
                    next_state = S_DONE;
                end else begin
                    next_state = S_COMPUTE;
                end
            end

            S_DONE: begin
                done = 1'b1;
                next_state = S_IDLE;
            end
        endcase
    end

    always_comb begin
        zr_sq = z_real * z_real;
        zi_sq = z_imag * z_imag;
        zr_zi = (z_real * z_imag) <<< 1;

        z_real_next = (zr_sq >>> FRAC_WIDTH) - (zi_sq >>> FRAC_WIDTH) + c_real_in;
        z_imag_next = (zr_zi >>> FRAC_WIDTH) + c_imag_in;
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            z_real       <= 0;
            z_imag       <= 0;
            current_iter <= 0;
        end else if (state == S_IDLE && next_state == S_COMPUTE) begin
            z_real       <= 0;
            z_imag       <= 0;
            current_iter <= 0;
        end else if (state == S_COMPUTE && next_state == S_COMPUTE) begin
            z_real       <= z_real_next;
            z_imag       <= z_imag_next;
            current_iter <= current_iter + 1;
        end
    end

    assign iteration_count_out = current_iter;

endmodule

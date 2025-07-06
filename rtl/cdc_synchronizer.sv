module cdc_synchronizer #(
    parameter WIDTH = 32
)(
    input                       dest_clk, // The clock in the destination domain
    input                       rst,
    input      [WIDTH-1:0]      data_in,  // Data from the source domain
    output reg [WIDTH-1:0]      data_out  // Synchronized data in the destination domain
);

    reg [WIDTH-1:0] sync_reg_1;

    always_ff @(posedge dest_clk) begin
        if (rst) begin
            sync_reg_1 <= '0;
            data_out   <= '0;
        end else begin
            sync_reg_1 <= data_in;
            data_out   <= sync_reg_1;
        end
    end

endmodule

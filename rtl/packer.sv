module packer(
    input           aclk,
    input           aresetn,

    // Pixel data input
    input [7:0]     r, g, b,
    
    // Control signals from main FSM
    input           valid,          // Input pixel is valid
    input           sof,            // Start of frame for this pixel
    input           eol,            // End of line for this pixel
    output logic    in_stream_ready,// Ready to accept a new pixel

    // AXI-Stream Output
    output [31:0]   out_stream_tdata,
    output [3:0]    out_stream_tkeep,
    output          out_stream_tlast,
    input           out_stream_tready,
    output          out_stream_tvalid,
    output          out_stream_tuser 
);

    localparam [1:0] STATE_IDLE = 2'b00;
    localparam [1:0] STATE_SEND = 2'b01;

    reg [1:0] state_reg, state_next;
    
    reg [23:0] data_to_send;
    reg        last_to_send;
    reg        user_to_send;

    wire input_fire  = valid && in_stream_ready;
    wire output_fire = out_stream_tvalid && out_stream_tready;

    always_comb begin
        state_next = state_reg;
        in_stream_ready = 1'b0;
        out_stream_tvalid = 1'b0;

        case(state_reg)
            STATE_IDLE: begin
                in_stream_ready = 1'b1;
                if (input_fire) begin
                    state_next = STATE_SEND;
                end
            end
            
            STATE_SEND: begin
                out_stream_tvalid = 1'b1;
                if (output_fire) begin
                    state_next = STATE_IDLE;
                end
            end
        endcase
    end

    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            state_reg <= STATE_IDLE;
            data_to_send <= 24'b0;
            last_to_send <= 1'b0;
            user_to_send <= 1'b0;
        end else begin
            state_reg <= state_next;
            
            if (input_fire) begin
                data_to_send <= {r, g, b};
                last_to_send <= eol;
                user_to_send <= sof;
            end
        end
    end

    // -- DEBUG
    // always @(posedge aclk) begin
    //     // Only print what we latch for the first or last pixel of a line
    //     if ( input_fire && (sof || eol) ) begin
    //         $display("[%0t] PACKER: ==> Latching interesting input! sof_in=%b, eol_in=%b. RGB=%h",
    //                 $time, sof, eol, {r,g,b});
    //     end
    // end

    // Assign outputs
    assign out_stream_tdata = {8'h00, data_to_send};
    assign out_stream_tkeep = 4'hF;
    assign out_stream_tlast = last_to_send;
    assign out_stream_tuser = user_to_send;

endmodule

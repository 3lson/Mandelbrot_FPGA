module pixel_generator(
    input           out_stream_aclk,
    input           s_axi_lite_aclk,
    input           axi_resetn,
    input           periph_resetn,

    //Stream output
    output [31:0]   out_stream_tdata,
    output [3:0]    out_stream_tkeep,
    output  logic       out_stream_tlast,
    input           out_stream_tready,
    output   logic       out_stream_tvalid,
    output   logic       out_stream_tuser, 

    //AXI-Lite S
    input [AXI_LITE_ADDR_WIDTH-1:0]     s_axi_lite_araddr,
    output          s_axi_lite_arready,
    input           s_axi_lite_arvalid,

    input [AXI_LITE_ADDR_WIDTH-1:0]     s_axi_lite_awaddr,
    output          s_axi_lite_awready,
    input           s_axi_lite_awvalid,

    input           s_axi_lite_bready,
    output [1:0]    s_axi_lite_bresp,
    output          s_axi_lite_bvalid,

    output [31:0]   s_axi_lite_rdata,
    input           s_axi_lite_rready,
    output [1:0]    s_axi_lite_rresp,
    output          s_axi_lite_rvalid,

    input  [31:0]   s_axi_lite_wdata,
    output          s_axi_lite_wready,
    input           s_axi_lite_wvalid

);

localparam X_SIZE = 640;
localparam Y_SIZE = 480;
parameter  REG_FILE_SIZE = 8;
localparam REG_FILE_AWIDTH = $clog2(REG_FILE_SIZE);
parameter  AXI_LITE_ADDR_WIDTH = 8;

localparam AWAIT_WADD_AND_DATA = 3'b000;
localparam AWAIT_WDATA = 3'b001;
localparam AWAIT_WADD = 3'b010;
localparam AWAIT_WRITE = 3'b100;
localparam AWAIT_RESP = 3'b101;

localparam AWAIT_RADD = 2'b00;
localparam AWAIT_FETCH = 2'b01;
localparam AWAIT_READ = 2'b10;

localparam AXI_OK = 2'b00;
localparam AXI_ERR = 2'b10;

reg [31:0]                          regfile [REG_FILE_SIZE-1:0];
reg [REG_FILE_AWIDTH-1:0]           writeAddr, readAddr;
reg [31:0]                          readData, writeData;
reg [1:0]                           readState = AWAIT_RADD;
reg [2:0]                           writeState = AWAIT_WADD_AND_DATA;

reg [AXI_LITE_ADDR_WIDTH-1:0]       axi_raddr_reg;
reg [AXI_LITE_ADDR_WIDTH-1:0]       axi_waddr_reg;

initial begin
    regfile[0] = 100;        // max_iter
    regfile[1] = 0;          // pan_x
    regfile[2] = 0;          // pan_y
    regfile[3] = 32'h10000000; // zoom = 1.0 in fixed point
    regfile[4] = 0;
    regfile[5] = 0;
    regfile[6] = 0;
    regfile[7] = 0;
end

//Read from the register file
always @(posedge s_axi_lite_aclk) begin
    
    readData <= regfile[readAddr];

    if (!axi_resetn) begin
        readState <= AWAIT_RADD;
        axi_raddr_reg <= 0;
    end else case (readState)
        AWAIT_RADD: begin
            if (s_axi_lite_arvalid) begin
                readAddr <= s_axi_lite_araddr[2+:REG_FILE_AWIDTH];
                axi_raddr_reg <= s_axi_lite_araddr;
                readState <= AWAIT_FETCH;
            end
        end

        AWAIT_FETCH: begin
            readState <= AWAIT_READ;
        end

        AWAIT_READ: begin
            if (s_axi_lite_rready) begin
                readState <= AWAIT_RADD;
            end
        end

        default: begin
            readState <= AWAIT_RADD;
        end
    endcase
end

assign s_axi_lite_arready = (readState == AWAIT_RADD);
assign s_axi_lite_rresp = (axi_raddr_reg < (REG_FILE_SIZE * 4)) ? AXI_OK : AXI_ERR;
assign s_axi_lite_rvalid = (readState == AWAIT_READ);
assign s_axi_lite_rdata = readData;

//Write to the register file
always @(posedge s_axi_lite_aclk) begin
    if (!axi_resetn) begin
        writeState <= AWAIT_WADD_AND_DATA;
        axi_waddr_reg <= 0;
    end else case (writeState)
        AWAIT_WADD_AND_DATA: begin
            case ({s_axi_lite_awvalid, s_axi_lite_wvalid})
                2'b10: begin // Address comes first
                    writeAddr <= s_axi_lite_awaddr[2+:REG_FILE_AWIDTH];
                    axi_waddr_reg <= s_axi_lite_awaddr;
                    writeState <= AWAIT_WDATA;
                end
                2'b01: begin // Data comes first
                    writeData <= s_axi_lite_wdata;
                    writeState <= AWAIT_WADD;
                end
                2'b11: begin // Both arrive together
                    writeData <= s_axi_lite_wdata;
                    writeAddr <= s_axi_lite_awaddr[2+:REG_FILE_AWIDTH];
                    axi_waddr_reg <= s_axi_lite_awaddr;
                    writeState <= AWAIT_WRITE;
                end
                default: begin
                    writeState <= AWAIT_WADD_AND_DATA;
                end
            endcase        
        end

        AWAIT_WDATA: begin
            if (s_axi_lite_wvalid) begin
                writeData <= s_axi_lite_wdata;
                writeState <= AWAIT_WRITE;
            end
        end

        AWAIT_WADD: begin
            if (s_axi_lite_awvalid) begin
                writeAddr <= s_axi_lite_awaddr[2+:REG_FILE_AWIDTH];
                axi_waddr_reg <= s_axi_lite_awaddr;
                writeState <= AWAIT_WRITE;
            end
        end

        AWAIT_WRITE: begin
            // Only write if address is valid to prevent corruption
            if (axi_waddr_reg < (REG_FILE_SIZE * 4)) begin
                regfile[writeAddr] <= writeData;
            end
            writeState <= AWAIT_RESP;
        end

        AWAIT_RESP: begin
            if (s_axi_lite_bready) begin
                writeState <= AWAIT_WADD_AND_DATA;
            end
        end

        default: begin
            writeState <= AWAIT_WADD_AND_DATA;
        end
    endcase
end

assign s_axi_lite_awready = (writeState == AWAIT_WADD_AND_DATA || writeState == AWAIT_WADD);
assign s_axi_lite_wready = (writeState == AWAIT_WADD_AND_DATA || writeState == AWAIT_WDATA);
assign s_axi_lite_bvalid = (writeState == AWAIT_RESP);
assign s_axi_lite_bresp = (axi_waddr_reg < (REG_FILE_SIZE * 4)) ? AXI_OK : AXI_ERR;

wire [31:0] max_iter_in = regfile[0];
wire [31:0] pan_x_in    = regfile[1];
wire [31:0] pan_y_in    = regfile[2];
wire [31:0] zoom_in     = regfile[3];

wire [31:0] max_iter_s;
wire [31:0] pan_x_s;
wire [31:0] pan_y_s;
wire [31:0] zoom_s;

// Instantiate synchronizers for each control signal
cdc_synchronizer #(.WIDTH(32)) sync_max_iter (
    .dest_clk(out_stream_aclk),
    .rst(!periph_resetn),
    .data_in(max_iter_in),
    .data_out(max_iter_s)
);

cdc_synchronizer #(.WIDTH(32)) sync_pan_x (
    .dest_clk(out_stream_aclk),
    .rst(!periph_resetn),
    .data_in(pan_x_in),
    .data_out(pan_x_s)
);

cdc_synchronizer #(.WIDTH(32)) sync_pan_y (
    .dest_clk(out_stream_aclk),
    .rst(!periph_resetn),
    .data_in(pan_y_in),
    .data_out(pan_y_s)
);

cdc_synchronizer #(.WIDTH(32)) sync_zoom (
    .dest_clk(out_stream_aclk),
    .rst(!periph_resetn),
    .data_in(zoom_in),
    .data_out(zoom_s)
);

// -- FSM State Definitions --
localparam FSM_START   = 2'd0;
localparam FSM_COMPUTE = 2'd1;
localparam FSM_VALID   = 2'd2;

// -- FSM and Control Registers --
reg [1:0] state = FSM_START;
reg [9:0] x = 0;
reg [8:0] y = 0;
reg start_mandel = 0;

reg sof_for_packer;
reg eol_for_packer;

wire pixel_valid;
assign pixel_valid = (state == FSM_VALID);

// -- Wires for connecting modules --
wire [31:0] c_re, c_im;
wire [31:0] iterations;
wire        mandel_ready;
wire [7:0]  r, g, b;
wire        packer_ready;

// -- Positional/Control Wires --
wire lastx = (x == X_SIZE - 1);
wire lasty = (y == Y_SIZE - 1);

// -- Control FSM --
always @(posedge out_stream_aclk) begin
    if (!periph_resetn) begin
        x <= 0;
        y <= 0;
        state <= FSM_START;
        start_mandel <= 0;
        sof_for_packer <= 0;
        eol_for_packer <= 0;
    end else begin
        start_mandel <= 0;

        case(state)
            FSM_START: begin
                //$display("In FSM_START");
                start_mandel <= 1; // Assert start for one cycle
                state <= FSM_COMPUTE;
            end

            FSM_COMPUTE: begin
                //$display("In FSM_COMPUTE: ", mandel_ready);
                if (mandel_ready) begin
                    state <= FSM_VALID;
                    sof_for_packer <= (x == 0 && y == 0);
                    eol_for_packer <= lastx;
                end
            end

            FSM_VALID: begin
                //$display("In FSM_VALID");
                if (packer_ready) begin // Wait for packer to accept the data
                    //$display("Pixel is valid and ready");
                    if (lastx) begin
                        x <= 0;
                        y <= y + 1;
                    end else begin
                        x <= x+ 1;
                    end
                    state <= FSM_START;
                end
            end
            default: state <= FSM_START;
        endcase
    end
end

// --- DEBUG
// always @(posedge out_stream_aclk) begin
//     // Only print on the first or last pixel of a line to reduce noise
//     if ( (pixel_valid && packer_ready) && (sof_for_packer || lastx) ) begin
//         $display("[%0t] PIXEL_GEN: Handshake for interesting pixel! (x=%0d, y=%0d). sof_for_packer=%b, lastx=%b, lasty=%b, tlast=%b",
//                  $time, x, y, sof_for_packer, lastx, lasty, out_stream_tlast);
//     end
// end

// -- Module Instantiations --

screen_mapper sm_inst (
    .x(x), .y(y),
    .pan_x(pan_x_s), .pan_y(pan_y_s), 
    .zoom(zoom_s[7:0]), 
    .c_re(c_re), .c_im(c_im)
);

mandelbrot_calculator mb_inst (
    .clk(out_stream_aclk), .rst(!periph_resetn),
    .start(start_mandel),
    .ready(mandel_ready),
    .c_re(c_re), .c_im(c_im), .max_iter(max_iter_s),
    .iterations(iterations)
);

color_mapper cm_inst (
    .clk(out_stream_aclk),
    .iterations_in(iterations),
    .max_iter(max_iter_s),
    .r(r), .g(g), .b(b)
);

packer pixel_packer(
    .aclk(out_stream_aclk),
    .aresetn(periph_resetn),
    .r(r), .g(g), .b(b),
    .eol(eol_for_packer), 
    .in_stream_ready(packer_ready), 
    .valid(pixel_valid), 
    .sof(sof_for_packer),
    .out_stream_tdata(out_stream_tdata), 
    .out_stream_tkeep(out_stream_tkeep),
    .out_stream_tlast(out_stream_tlast), 
    .out_stream_tready(out_stream_tready),
    .out_stream_tvalid(out_stream_tvalid), 
    .out_stream_tuser(out_stream_tuser) 
);
 
endmodule

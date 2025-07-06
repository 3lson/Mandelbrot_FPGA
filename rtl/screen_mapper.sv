module screen_mapper (
    // Inputs
    input [9:0]  x,
    input [9:0]  y,
    input [31:0] pan_x, 
    input [31:0] pan_y,
    input [7:0]  zoom,

    output logic [31:0] c_re,
    output logic [31:0] c_im
);

    localparam H_CENTER = 640 / 2; 
    localparam V_CENTER = 480 / 2;

    wire signed [11:0] x_centered = $signed({2'b00, x}) - H_CENTER;
    wire signed [11:0] y_centered = $signed({2'b00, y}) - V_CENTER;
    
    wire signed [35:0] x_fixed = $signed(x_centered) <<< 24;
    wire signed [35:0] y_fixed = $signed(y_centered) <<< 24;
    
    wire [4:0] zoom_limited = (zoom > 5'd24) ? 5'd24 : zoom[4:0];
    wire signed [35:0] x_zoomed = x_fixed >>> zoom_limited;
    wire signed [35:0] y_zoomed = y_fixed >>> zoom_limited;
    
    wire signed [31:0] x_scaled = x_zoomed[35:4];  
    wire signed [31:0] y_scaled = y_zoomed[35:4];
    
    assign c_re = x_scaled + $signed(pan_x);
    assign c_im = y_scaled + $signed(pan_y);

endmodule

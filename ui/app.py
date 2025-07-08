# app.py - Updated with CPU Rendering Path
import sys
import os
import time
import numpy as np
from flask import Flask, render_template, request, jsonify
from numba import jit
from PIL import Image
import io
import base64

# This allows us to import from the same directory
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

# We can keep your utility module, but we won't use it for the CPU version
from mandelbrot_utils import calculate_hw_params

app = Flask(__name__)

# =================================================================
# --- HARDWARE PLACEHOLDERS (To be replaced with PYNQ code) ---
# =================================================================

def initialize_hardware():
    """On the PYNQ board, this function will load the overlay."""
    print("MOCK: Initializing hardware...")
    # global overlay, s2mm_channel, mandel_ip
    # from pynq import Overlay
    # from pynq.lib.video import VideoMode
    # overlay = Overlay('elec.bit')
    # s2mm_channel = overlay.video.axi_vdma_0.readchannel
    # mandel_ip = overlay.pixel_generator_0
    print("MOCK: Hardware initialized.")

def generate_mandelbrot_fpga(ui_state):
    """This function will run the hardware and return a frame."""
    print("MOCK: Calling FPGA generation...")
    # 1. Configure the capture channel
    # VIDEO_MODE = VideoMode(640, 480, 24)
    # s2mm_channel.mode = VIDEO_MODE
    # s2mm_channel.start()
    
    # 2. Set parameters on the IP
    # hw_params = calculate_hw_params(ui_state) # Use your existing conversion
    # mandel_ip.write(0x00, hw_params['max_iter'])
    # mandel_ip.write(0x04, ...) # etc. for pan and zoom
    
    # 3. Read the frame back
    # frame = s2mm_channel.readframe()
    # s2mm_channel.stop()
    
    # For now, return a placeholder black frame
    return np.zeros((480, 640, 3), dtype=np.uint8)

# =================================================================
# --- SOFTWARE (CPU) IMPLEMENTATION ---
# =================================================================

@jit(nopython=True) # This decorator compiles the function to machine code
def mandelbrot_cpu_pixel(c_re, c_im, max_iter):
    """Calculates iteration count for a single pixel on the CPU."""
    z_re, z_im = 0.0, 0.0
    for i in range(max_iter):
        z_re_sq, z_im_sq = z_re * z_re, z_im * z_im
        if z_re_sq + z_im_sq > 4.0:
            return i
        z_im = 2 * z_re * z_im + c_im
        z_re = z_re_sq - z_im_sq + c_re
    return max_iter

def generate_mandelbrot_cpu(ui_state):
    """Generates a full Mandelbrot frame using the CPU."""
    # Extract parameters from the UI state
    width, height = 640, 480
    max_iter = ui_state.get('maxIter', 100)
    center_x = ui_state.get('centerX', -0.7)
    center_y = ui_state.get('centerY', 0.0)
    zoom = ui_state.get('zoom', 1.0)
    
    # Calculate view dimensions
    view_width = 3.5 / zoom
    view_height = view_width * (height / width)
    
    # Create an empty image buffer
    frame = np.zeros((height, width, 3), dtype=np.uint8)
    
    # Iterate over every pixel, calculate, and color it
    for y in range(height):
        for x in range(width):
            c_re = center_x - (view_width / 2.0) + (x / width) * view_width
            c_im = center_y - (view_height / 2.0) + (y / height) * view_height
            
            iters = mandelbrot_cpu_pixel(c_re, c_im, max_iter)
            
            # Simple color mapping for demonstration
            if iters == max_iter:
                frame[y, x] = [0, 0, 0]
            else:
                color_val = int(255 * (iters / max_iter))
                frame[y, x] = [color_val, color_val % 128, 255 - color_val]
                
    return frame

# =================================================================
# --- FLASK WEB ROUTES ---
# =================================================================

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update', methods=['POST'])
def update_fractal():
    ui_state = request.get_json()
    render_mode = ui_state.get('renderMode', 'fpga')
    
    start_time = time.perf_counter()
    
    if render_mode == 'cpu':
        frame = generate_mandelbrot_cpu(ui_state)
        mode_used = "CPU"
    else: # Default to FPGA
        frame = generate_mandelbrot_fpga(ui_state)
        mode_used = "FPGA (MOCK)"
    
    end_time = time.perf_counter()
    
    # Convert NumPy image to a Base64 string to send in JSON
    pil_img = Image.fromarray(frame)
    buff = io.BytesIO()
    pil_img.save(buff, format="PNG")
    img_base64 = base64.b64encode(buff.getvalue()).decode("utf-8")
    
    # Calculate performance metrics
    delay = end_time - start_time
    fps = 1.0 / delay if delay > 0 else 0
    
    return jsonify({
        "status": "ok",
        "fps": f"{fps:.2f}",
        "renderTime": f"{delay:.3f}s",
        "throughput": f"{(640*480)/delay/1e6:.2f} MPixels/s",
        "modeUsed": mode_used,
        "imageBase64": f"data:image/png;base64,{img_base64}"
    })

if __name__ == '__main__':
    # When you run on PYNQ, you will uncomment this line.
    # initialize_hardware()
    app.run(host='0.0.0.0', port=5000, debug=True)
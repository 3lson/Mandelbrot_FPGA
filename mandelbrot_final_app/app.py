# app.py - FINAL, CLEANED INDENTATION

import sys
import os
import time
import numpy as np
from flask import Flask, render_template, request, jsonify
from PIL import Image
import io
import base64

# --- PYNQ Imports ---
try:
    from pynq import Overlay
    from pynq.lib.video import VideoMode
    PYNQ_AVAILABLE = True
except ImportError:
    PYNQ_AVAILABLE = False

# This allows us to import from the same directory
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

from mandelbrot_utils import calculate_hw_params

app = Flask(__name__)

# =================================================================
# --- GLOBAL HARDWARE HANDLES ---
# =================================================================
overlay = None
s2mm_channel = None
mandel_ip = None
VIDEO_MODE = None

# =================================================================
# --- HARDWARE IMPLEMENTATION ---
# =================================================================
def initialize_hardware():
    """Loads the overlay and gets handles to our IP. Called once on startup."""
    global overlay, s2mm_channel, mandel_ip, VIDEO_MODE
    if not PYNQ_AVAILABLE:
        print("PYNQ libraries not found. Running in software-only mode.")
        return
    print("Initializing hardware... This may take a moment.")
    try:
        overlay = Overlay('elec.bit')
        s2mm_channel = overlay.video.axi_vdma_0.readchannel
        mandel_ip = overlay.pixel_generator_0
        VIDEO_MODE = VideoMode(640, 480, 24)
        print("Hardware initialized successfully!")
    except Exception as e:
        print(f"Error initializing hardware: {e}")
        print("The application will continue in software-only mode.")

def float_to_q4_28(val):
    """Helper function for fixed-point conversion."""
    return int(val * (2**28))

def generate_mandelbrot_fpga(ui_state):
    """
    Configures the Mandelbrot IP, captures one frame from the hardware,
    and returns it as a NumPy array.
    """
    if not mandel_ip or not s2mm_channel:
        print("FPGA not available, returning black frame.")
        return np.zeros((480, 640, 3), dtype=np.uint8)
    s2mm_channel.mode = VIDEO_MODE
    s2mm_channel.start()
    pan_x, pan_y = ui_state.get('centerX', -0.75), ui_state.get('centerY', 0.0)
    zoom, max_iter = ui_state.get('zoom', 1.0), ui_state.get('maxIter', 100)
    zoom_level = int(np.log2(zoom)) if zoom > 0 else 0
    mandel_ip.write(0x00, max_iter)
    mandel_ip.write(0x04, float_to_q4_28(pan_x))
    mandel_ip.write(0x08, float_to_q4_28(pan_y))
    mandel_ip.write(0x0C, zoom_level)
    frame = s2mm_channel.readframe()
    s2mm_channel.stop()
    return frame

# =================================================================
# --- SOFTWARE (CPU) IMPLEMENTATION ---
# =================================================================
def mandelbrot_cpu_pixel(c_re, c_im, max_iter):
    z_re, z_im = 0.0, 0.0
    for i in range(max_iter):
        z_re_sq, z_im_sq = z_re * z_re, z_im * z_im
        if z_re_sq + z_im_sq > 4.0: return i
        z_im = 2 * z_re * z_im + c_im
        z_re = z_re_sq - z_im_sq + c_re
    return max_iter

def generate_mandelbrot_cpu(ui_state):
    width, height = 640, 480
    max_iter = ui_state.get('maxIter', 100)
    center_x, center_y = ui_state.get('centerX', -0.7), ui_state.get('centerY', 0.0)
    zoom = ui_state.get('zoom', 1.0)
    view_width = 3.5 / zoom
    view_height = view_width * (height / width)
    frame = np.zeros((height, width, 3), dtype=np.uint8)
    for y in range(height):
        for x in range(width):
            c_re = center_x - view_width/2 + (x/width)*view_width
            c_im = center_y - view_height/2 + (y/height)*view_height
            iters = mandelbrot_cpu_pixel(c_re, c_im, max_iter)
            if iters == max_iter:
                frame[y, x] = [0, 0, 0]
            else:
                c = int(255 * (iters / 80))
                frame[y, x] = [min(c, 255), min(c*2, 255) % 255, 255]
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
    else:
        frame = generate_mandelbrot_fpga(ui_state)
        mode_used = "FPGA"
    end_time = time.perf_counter()
    pil_img = Image.fromarray(frame)
    buff = io.BytesIO()
    pil_img.save(buff, format="PNG")
    img_base64 = base64.b64encode(buff.getvalue()).decode("utf-8")
    delay = end_time - start_time
    fps = 1.0 / delay if delay > 0 else 0
    return jsonify({
        "status": "ok", "fps": f"{fps:.2f}", "renderTime": f"{delay:.3f}s",
        "throughput": f"{(640*480)/delay/1e6:.2f} MPixels/s",
        "modeUsed": mode_used, "imageBase64": f"data:image/png;base64,{img_base64}"
    })

if __name__ == '__main__':
    initialize_hardware()
    # Force Flask to be single-threaded for stability with PYNQ
    app.run(host='0.0.0.0', port=5000, threaded=False)
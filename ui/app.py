import sys
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

import time
from flask import Flask, render_template, request, jsonify
from mandelbrot_utils import calculate_hw_params # Import your new module
import random

app = Flask(__name__)

# --- Global Hardware Handles (will be initialized at startup) ---
# overlay = None
# pixel_gen = None
# hdmi_out = None

def initialize_hardware():
    """Function to load the overlay and get IP handles. Will be called once."""
    global overlay, pixel_gen, hdmi_out
    print("Initializing hardware... (MOCK)")
    # --- On PYNQ, this will be real ---
    # overlay = Overlay('pixel_generator.bit')
    # pixel_gen = overlay.pixel_generator_0
    # hdmi_out = overlay.video.hdmi_out
    # videomode = VideoMode(640, 480, 24)
    # hdmi_out.configure(videomode)
    # hdmi_out.start()
    print("Hardware initialized.")

def set_mandelbrot_params(params):
    """Writes the calculated hardware parameters to the FPGA registers."""
    print("Setting Mandelbrot parameters:")
    for key, value in params.items():
        print(f"  Writing {key} = {value}")
    
    # --- On PYNQ, this will be real MMIO writes ---
    # pixel_gen.mmio.write(0 * 4, params['max_iter'])
    # pixel_gen.mmio.write(1 * 4, params['c_real_min'])
    # pixel_gen.mmio.write(2 * 4, params['c_imag_max'])
    # pixel_gen.mmio.write(3 * 4, params['scale'])

    # The hardware starts generating automatically once parameters are set.
    # We can measure the time it takes for a frame to be "ready" if we had a status register.
    # For now, we'll just simulate a delay.
    time.sleep(0.05 + (params['max_iter'] / 50000.0)) # Mock delay


@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update', methods=['POST'])
def update_fractal():
    ui_state = request.get_json()
    
    # 1. Convert UI state to hardware parameters using our utility function
    hw_params = calculate_hw_params(ui_state)
    
    # 2. Set the parameters on the hardware
    start_time = time.time()
    set_mandelbrot_params(hw_params)
    end_time = time.time()

    # 3. Return performance metrics
    delay = end_time - start_time
    fps = 1.0 / delay if delay > 0 else 0
    return jsonify({
        "status": "ok",
        "fps": f"{fps:.2f}",
        "renderTime": f"{delay:.3f}s",
        "throughput": f"{(640*480)/delay/1e6:.2f} MPixels/s"
    })

if __name__ == '__main__':
    # initialize_hardware() # Call this once on startup
    app.run(host='0.0.0.0', port=5000, debug=True)
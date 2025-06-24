# Screen and Mandelbrot Set Constants
SCREEN_WIDTH = 640
SCREEN_HEIGHT = 480
BASE_VIEW_WIDTH = 3.5 # The complex plane width at zoom = 1.0

def float_to_q4_28(val):
    """Converts a Python float to a Q4.28 fixed-point integer."""
    return int(val * (2**28))

def calculate_hw_params(ui_state):
    """
    Converts UI state (centerX, centerY, zoom) into hardware parameters
    (c_real_min, c_imag_max, scale) in fixed-point format.
    """
    zoom = ui_state.get('zoom', 1.0)
    center_x = ui_state.get('centerX', -0.7)
    center_y = ui_state.get('centerY', 0.0)
    
    # 1. Calculate view dimensions in the complex plane
    view_width = BASE_VIEW_WIDTH / zoom
    view_height = view_width * (SCREEN_HEIGHT / SCREEN_WIDTH)
    
    # 2. Calculate the top-left corner of the view
    c_real_min_f = center_x - (view_width / 2.0)
    c_imag_max_f = center_y + (view_height / 2.0)
    
    # 3. Calculate the scale factor
    scale_f = view_width / SCREEN_WIDTH
    
    # 4. Convert all floating-point values to Q4.28 fixed-point integers
    hw_params = {
        'max_iter': int(ui_state.get('maxIter', 100)),
        'c_real_min': float_to_q4_28(c_real_min_f),
        'c_imag_max': float_to_q4_28(c_imag_max_f),
        'scale': float_to_q4_28(scale_f)
    }
    
    return hw_params

# --- You can test this function right now! ---
if __name__ == '__main__':
    # Simulate the UI sending a state
    mock_ui_state = {
        'centerX': -0.745,
        'centerY': 0.186,
        'zoom': 500.0,
        'maxIter': 1000
    }
    
    hardware_parameters = calculate_hw_params(mock_ui_state)
    
    print("UI State:", mock_ui_state)
    print("-" * 20)
    print("Calculated Hardware Parameters (as integers):")
    for key, value in hardware_parameters.items():
        print(f"  {key}: {value}")
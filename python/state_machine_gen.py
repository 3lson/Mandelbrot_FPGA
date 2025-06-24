import graphviz

def create_mandelbrot_state_machine():
    # Create a new directed graph
    dot = graphviz.Digraph(comment='Mandelbrot State Machine')
    dot.attr(rankdir='LR')  # Left to right layout
    dot.attr('node', shape='circle', style='filled')
    
    # Define states
    dot.node('S_IDLE', 'S_IDLE\n(Waiting)')
    dot.node('S_COMPUTE', 'S_COMPUTE\n(Iterating)')
    dot.node('S_DONE', 'S_DONE\n(Complete)')
    
    # Define transitions
    # From S_IDLE
    dot.edge('S_IDLE', 'S_COMPUTE', label='start = 1')
    dot.edge('S_IDLE', 'S_IDLE', label='start = 0')
    
    # From S_COMPUTE
    dot.edge('S_COMPUTE', 'S_DONE', label='escape condition OR\nmax iterations')
    dot.edge('S_COMPUTE', 'S_COMPUTE', label='continue iterating')
    
    # From S_DONE
    dot.edge('S_DONE', 'S_IDLE', label='automatic\n(done = 1)')
    
    # Add reset transitions (global)
    dot.edge('S_COMPUTE', 'S_IDLE', label='rst', constraint='false')
    dot.edge('S_DONE', 'S_IDLE', label='rst', constraint='false')
    
    return dot

def create_detailed_mandelbrot_state_machine():
    """More detailed version with conditions and actions"""
    dot = graphviz.Digraph(comment='Detailed Mandelbrot State Machine')
    dot.attr(rankdir='TB')  # Top to bottom layout
    dot.attr('node', shape='record', style='filled')
    
    # Define states with detailed information
    idle_label = '{S_IDLE|Actions:\\l• done = 0\\l• Wait for start\\l}'
    compute_label = '{S_COMPUTE|Actions:\\l• Calculate z² + c\\l• Increment iteration\\l• Check escape condition\\l}'
    done_label = '{S_DONE|Actions:\\l• done = 1\\l• Output final count\\l}'
    
    dot.node('S_IDLE', idle_label)
    dot.node('S_COMPUTE', compute_label)
    dot.node('S_DONE', done_label)
    
    # Transitions with detailed conditions
    dot.edge('S_IDLE', 'S_COMPUTE', 
             label='start = 1\\n\\nInitialize:\\n• z_real = 0\\n• z_imag = 0\\n• current_iter = 0')
    
    dot.edge('S_COMPUTE', 'S_COMPUTE', 
             label='|z²| < 4 AND\\ncurrent_iter < max_iter\\n\\nUpdate:\\n• z = z² + c\\n• iter++')
    
    dot.edge('S_COMPUTE', 'S_DONE', 
             label='|z²| ≥ 4 OR\\ncurrent_iter ≥ max_iter')
    
    dot.edge('S_DONE', 'S_IDLE', label='Next clock cycle')
    
    # Reset (global)
    dot.attr('edge', style='dashed', color='red')
    dot.edge('S_COMPUTE', 'S_IDLE', label='rst', constraint='false')
    dot.edge('S_DONE', 'S_IDLE', label='rst', constraint='false')
    
    return dot

# Create and display/save DOT source
if __name__ == "__main__":
    # Simple version
    simple_fsm = create_mandelbrot_state_machine()
    
    # Detailed version
    detailed_fsm = create_detailed_mandelbrot_state_machine()
    
    # Save DOT files
    with open('mandelbrot_fsm_simple.dot', 'w') as f:
        f.write(simple_fsm.source)
    
    with open('mandelbrot_fsm_detailed.dot', 'w') as f:
        f.write(detailed_fsm.source)
    
    print("DOT files generated:")
    print("- mandelbrot_fsm_simple.dot")
    print("- mandelbrot_fsm_detailed.dot")
    print("\nYou can:")
    print("1. Install Graphviz: brew install graphviz")
    print("2. Use online DOT viewer: http://magjac.com/graphviz-visual-editor/")
    print("3. Use VS Code with Graphviz Preview extension")
    
    # Try to render if Graphviz is available
    try:
        simple_fsm.render('mandelbrot_fsm_simple', format='png', cleanup=True)
        detailed_fsm.render('mandelbrot_fsm_detailed', format='png', cleanup=True)
        print("\nRendered PNG files:")
        print("- mandelbrot_fsm_simple.png")
        print("- mandelbrot_fsm_detailed.png")
    except Exception as e:
        print(f"\nCould not render images: {e}")
        print("Install Graphviz system package to generate images")
    
    # Print DOT source
    print("\n" + "="*50)
    print("Simple FSM DOT source:")
    print("="*50)
    print(simple_fsm.source)
    
    print("\n" + "="*50)
    print("Detailed FSM DOT source:")
    print("="*50)
    print(detailed_fsm.source)
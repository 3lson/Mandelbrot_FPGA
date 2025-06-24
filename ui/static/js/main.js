document.addEventListener('DOMContentLoaded', () => {
    // --- Get references to all UI elements ---
    const iterSlider = document.getElementById('iter-slider');
    const iterValue = document.getElementById('iter-value');
    const precisionSlider = document.getElementById('precision-slider');
    const precisionValue = document.getElementById('precision-value');
    const colorSchemeSelect = document.getElementById('color-scheme');
    const renderModeRadios = document.querySelectorAll('input[name="renderMode"]');
    const presetButtons = document.querySelectorAll('.btn-preset');
    const resetButton = document.getElementById('btn-reset');
    const displayMock = document.querySelector('.mandelbrot-display-mock');
    const loadingSpinner = document.getElementById('loading-spinner');
    const howItWorksButton = document.getElementById('btn-how-it-works');
    const modalContainer = document.getElementById('modal-container');
    const modalCloseButton = document.getElementById('modal-close');
    const themeToggleButton = document.getElementById('btn-theme-toggle');

    const expMode = document.getElementById('exp-mode');
    const expIter = document.getElementById('exp-iter');
    const expZoom = document.getElementById('exp-zoom');
        
    // Performance metric spans
    const metricMode = document.getElementById('metric-mode');
    const metricTime = document.getElementById('metric-time');
    const metricFps = document.getElementById('metric-fps');
    const metricThroughput = document.getElementById('metric-throughput');

    // --- State Management ---
    // A single object to hold the current state of our view
    const viewState = {
        centerX: -0.7,
        centerY: 0.0,
        zoom: 1.0,
        maxIter: parseInt(iterSlider.value),
        precision: parseInt(precisionSlider.value),
        colorScheme: colorSchemeSelect.value,
        renderMode: document.querySelector('input[name="renderMode"]:checked').value,
    };

    const updateLiveExplanation = () => {
        const mode = viewState.renderMode === 'fpga' ? 'the parallel FPGA hardware' : 'the sequential CPU';
        expMode.textContent = `▶ Calculations are being performed by ${mode}.`;

        expIter.textContent = `▶ The system will check up to ${viewState.maxIter} times per pixel to see if it escapes.`;

        if (viewState.zoom > 1.0) {
            expZoom.textContent = `▶ You are magnifying a region of the set by ${viewState.zoom.toFixed(1)}x. This requires high-precision math.`;
        } else {
            expZoom.textContent = `▶ You are viewing the entire Mandelbrot set.`;
        }
    };

    // --- The Main Update Function ---
    // This function sends the current state to the backend and updates the UI
    const updateView = async () => {
        console.log('Sending state to backend:', viewState);
        updateLiveExplanation();
        loadingSpinner.classList.remove('spinner-hidden');

        try {
            const response = await fetch('/update', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(viewState),
            });
            const data = await response.json();

            // Update performance metrics on the screen
            metricMode.textContent = data.modeUsed;
            metricTime.textContent = data.renderTime;
            metricFps.textContent = data.fps;
            metricThroughput.textContent = data.throughput;

        } catch (error) {
            console.error('Error updating view:', error);
            metricMode.textContent = "Error";
        } finally {
            loadingSpinner.classList.add('spinner-hidden');
        }
    };

    // --- Event Listeners ---
    iterSlider.addEventListener('input', () => {
        iterValue.textContent = iterSlider.value;
    });
    iterSlider.addEventListener('change', () => {
        viewState.maxIter = parseInt(iterSlider.value);
        updateView();
    });

    precisionSlider.addEventListener('input', () => {
        precisionValue.textContent = `${precisionSlider.value}-bit`;
    });
    precisionSlider.addEventListener('change', () => {
        viewState.precision = parseInt(precisionSlider.value);
        updateView();
    });
    
    colorSchemeSelect.addEventListener('change', () => {
        viewState.colorScheme = colorSchemeSelect.value;
        updateView();
    });

    renderModeRadios.forEach(radio => {
        radio.addEventListener('change', () => {
            viewState.renderMode = radio.value;
            updateView();
        });
    });

    resetButton.addEventListener('click', () => {
        viewState.centerX = -0.7;
        viewState.centerY = 0.0;
        viewState.zoom = 1.0;
        updateView();
    });

    presetButtons.forEach(button => {
        button.addEventListener('click', () => {
            const preset = button.dataset.preset;
            if (preset === 'seahorse') {
                viewState.centerX = -0.745;
                viewState.centerY = 0.186;
                viewState.zoom = 500.0;
            } else if (preset === 'elephant') {
                viewState.centerX = 0.374;
                viewState.centerY = 0.214;
                viewState.zoom = 200.0;
            } else if (preset === 'spiral') {
                viewState.centerX = -0.748;
                viewState.centerY = 0.098;
                viewState.zoom = 10000.0;
            }
            updateView();
        });
    });

    displayMock.addEventListener('click', () => {
        viewState.zoom *= 2.0; // Zoom in
        updateView();
    });
    
    document.addEventListener('keydown', (e) => {
        const panAmount = 0.2 / viewState.zoom;
        if(e.key === 'ArrowUp') viewState.centerY += panAmount;
        else if(e.key === 'ArrowDown') viewState.centerY -= panAmount;
        else if(e.key === 'ArrowLeft') viewState.centerX -= panAmount;
        else if(e.key === 'ArrowRight') viewState.centerX += panAmount;
        else return; // Don't update if it's not an arrow key
        updateView();
    });

    // --- Modal Logic ---
    howItWorksButton.addEventListener('click', () => {
        modalContainer.classList.add('modal-active');
    });

    modalCloseButton.addEventListener('click', () => {
        modalContainer.classList.remove('modal-active');
    });

    modalContainer.addEventListener('click', (event) => {
        // Close modal if user clicks on the dark background
        if (event.target === modalContainer) {
            modalContainer.classList.remove('modal-active');
        }
    });

    mok// --- Theme Toggle Logic ---
    const applyTheme = (theme) => {
        if (theme === 'dark') {
            document.body.classList.add('dark-mode');
            themeToggleButton.textContent = 'Light Mode';
        } else {
            document.body.classList.remove('dark-mode');
            themeToggleButton.textContent = 'Dark Mode';
        }
    };

    themeToggleButton.addEventListener('click', () => {
        // Check if dark mode is currently active
        const isDarkMode = document.body.classList.contains('dark-mode');
        // If it is, switch to light. Otherwise, switch to dark.
        const newTheme = isDarkMode ? 'light' : 'dark';
        localStorage.setItem('theme', newTheme);
        applyTheme(newTheme);
    });

    // --- On Page Load: Check for a saved theme ---
    const savedTheme = localStorage.getItem('theme') || 'light'; // Default to light
    applyTheme(savedTheme);

    // --- Initial Render ---
    updateView();
});
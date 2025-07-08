// main.js - Updated to handle image display
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
    const infoOverlay = document.querySelector('.info-overlay'); // Get the info overlay
    const howItWorksButton = document.getElementById('btn-how-it-works');
    const modalContainer = document.getElementById('modal-container');
    const modalCloseButton = document.getElementById('modal-close');
    const themeToggleButton = document.getElementById('btn-theme-toggle');

    const expMode = document.getElementById('exp-mode');
    const expIter = document.getElementById('exp-iter');
    const expZoom = document.getElementById('exp-zoom');
        
    const metricMode = document.getElementById('metric-mode');
    const metricTime = document.getElementById('metric-time');
    const metricFps = document.getElementById('metric-fps');
    const metricThroughput = document.getElementById('metric-throughput');

    // --- State Management ---
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
    const updateView = async () => {
        console.log('Sending state to backend:', viewState);
        updateLiveExplanation();
        loadingSpinner.classList.remove('spinner-hidden');

        try {
            const response = await fetch('/update', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(viewState),
            });
            const data = await response.json();

            metricMode.textContent = data.modeUsed;
            metricTime.textContent = data.renderTime;
            metricFps.textContent = data.fps;
            metricThroughput.textContent = data.throughput;

            // ***** NEW CODE HERE *****
            // Check if the backend sent us image data
            if (data.imageBase64) {
                // Set the background of the display div to our new image
                displayMock.style.backgroundImage = `url('${data.imageBase64}')`;
                // Hide the placeholder text
                infoOverlay.style.display = 'none';
            }

        } catch (error) {
            console.error('Error updating view:', error);
            metricMode.textContent = "Error";
        } finally {
            loadingSpinner.classList.add('spinner-hidden');
        }
    };

    // --- Event Listeners (No changes needed below this line) ---
    iterSlider.addEventListener('input', () => { iterValue.textContent = iterSlider.value; });
    iterSlider.addEventListener('change', () => { viewState.maxIter = parseInt(iterSlider.value); updateView(); });

    precisionSlider.addEventListener('input', () => { precisionValue.textContent = `${precisionSlider.value}-bit`; });
    precisionSlider.addEventListener('change', () => { viewState.precision = parseInt(precisionSlider.value); updateView(); });
    
    colorSchemeSelect.addEventListener('change', () => { viewState.colorScheme = colorSchemeSelect.value; updateView(); });

    renderModeRadios.forEach(radio => {
        radio.addEventListener('change', () => { viewState.renderMode = radio.value; updateView(); });
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
            if (preset === 'seahorse') { viewState.centerX = -0.745; viewState.centerY = 0.186; viewState.zoom = 500.0; } 
            else if (preset === 'elephant') { viewState.centerX = 0.374; viewState.centerY = 0.214; viewState.zoom = 200.0; } 
            else if (preset === 'spiral') { viewState.centerX = -0.748; viewState.centerY = 0.098; viewState.zoom = 10000.0; }
            updateView();
        });
    });

    displayMock.addEventListener('click', (e) => {
        const rect = displayMock.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        const width = rect.width;
        const height = rect.height;

        // Convert click coordinates to a shift in the view center
        const viewWidth = 3.5 / viewState.zoom;
        const viewHeight = viewWidth * (height/width);

        viewState.centerX += ((x / width) - 0.5) * viewWidth;
        viewState.centerY += ((y / height) - 0.5) * viewHeight;
        viewState.zoom *= 2.0;
        updateView();
    });
    
    document.addEventListener('keydown', (e) => {
        const panAmount = 0.2 / viewState.zoom;
        if(e.key === 'ArrowUp') viewState.centerY -= panAmount;
        else if(e.key === 'ArrowDown') viewState.centerY += panAmount;
        else if(e.key === 'ArrowLeft') viewState.centerX -= panAmount;
        else if(e.key === 'ArrowRight') viewState.centerX += panAmount;
        else return;
        e.preventDefault();
        updateView();
    });

    // Modal and Theme logic remains the same
    howItWorksButton.addEventListener('click', () => modalContainer.classList.add('modal-active'));
    modalCloseButton.addEventListener('click', () => modalContainer.classList.remove('modal-active'));
    modalContainer.addEventListener('click', (event) => { if (event.target === modalContainer) modalContainer.classList.remove('modal-active'); });
    
    const applyTheme = (theme) => {
        document.body.classList.toggle('dark-mode', theme === 'dark');
        themeToggleButton.textContent = theme === 'dark' ? 'Light Mode' : 'Dark Mode';
    };
    themeToggleButton.addEventListener('click', () => {
        const newTheme = document.body.classList.contains('dark-mode') ? 'light' : 'dark';
        localStorage.setItem('theme', newTheme);
        applyTheme(newTheme);
    });
    const savedTheme = localStorage.getItem('theme') || 'light';
    applyTheme(savedTheme);

    // Initial Render
    updateView();
});
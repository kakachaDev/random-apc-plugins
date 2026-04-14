import * as Juce from "./juce/index.js";

// ============================================================
// CLOUDWASH - JUCE INTEGRATION (FIXED VERSION)
// Robust parameter binding with graceful degradation
// ============================================================

console.log("CloudWash WebView UI Loading...");

// Parameter state management
const parameterStates = {
    position: null,
    size: null,
    pitch: null,
    density: null,
    texture: null,
    in_gain: null,
    blend: null,
    spread: null,
    feedback: null,
    reverb: null,
    mode: null,
    freeze: null,
    quality: null,
    sample_mode: null
};

// Track initialization status
let juceAvailable = false;
let uiInitialized = false;

document.addEventListener("DOMContentLoaded", () => {
    console.log("=== CloudWash Initialization Start ===");

    try {
        // Check JUCE availability
        juceAvailable = typeof window.__JUCE__ !== 'undefined';
        console.log(`JUCE Backend: ${juceAvailable ? '✓ Available' : '✗ Not Available'}`);

        // ALWAYS initialize UI (visual rendering doesn't require JUCE)
        console.log("Initializing visual controls...");
        initializeKnobs();
        initializeModeTabs();
        initializeFreezeButton();
        initializeDropdowns();
        initializeMeters();
        initializeGrainVisualization();

        uiInitialized = true;
        console.log("✓ Visual controls initialized");

        // Initialize JUCE parameter bindings (can fail gracefully)
        if (juceAvailable) {
            console.log("Connecting JUCE parameters...");
            initializeParameterStates();
            console.log("✓ JUCE parameters connected");
        } else {
            console.warn("Running in standalone mode - parameters won't sync with plugin");
        }

        console.log("=== CloudWash Ready ===");
    } catch (error) {
        console.error("=== CloudWash Initialization Error ===");
        console.error("Error details:", error);
        console.error("Stack trace:", error.stack);

        // Show error to user
        document.body.innerHTML = `
            <div style="padding: 40px; color: #ff4444; font-family: monospace;">
                <h2>CloudWash Initialization Error</h2>
                <p><strong>Error:</strong> ${error.message}</p>
                <p><strong>Stack:</strong></p>
                <pre>${error.stack}</pre>
            </div>
        `;
    }
});

function initializeParameterStates() {
    if (!juceAvailable) {
        console.warn("Skipping parameter state initialization - JUCE not available");
        return;
    }

    let successCount = 0;
    let failCount = 0;

    // Initialize JUCE parameter state objects
    for (const paramName in parameterStates) {
        try {
            if (paramName === 'freeze') {
                parameterStates[paramName] = Juce.getToggleState(paramName);
            } else {
                parameterStates[paramName] = Juce.getSliderState(paramName);
            }
            successCount++;
            console.log(`  ✓ ${paramName}`);
        } catch (error) {
            failCount++;
            console.warn(`  ✗ ${paramName}: ${error.message}`);
        }
    }

    console.log(`Parameter states: ${successCount} success, ${failCount} failed`);
}

function initializeKnobs() {
    const knobs = document.querySelectorAll('.knob');
    console.log(`Initializing ${knobs.length} knobs...`);

    const ARC_START = 120;  // Knobs rotated -90° (gap at bottom)
    const ARC_RANGE = 300;
    const KNOB_RADIUS = 31;  // For 70px diameter knob

    knobs.forEach((knob, index) => {
        try {
            const paramName = knob.dataset.param;
            const min = parseFloat(knob.dataset.min);
            const max = parseFloat(knob.dataset.max);
            const defaultValue = parseFloat(knob.dataset.default);

            let value = defaultValue;
            let isDragging = false;
            let startY = 0;
            let startValue = 0;

            const track = knob.querySelector('.knob-track');
            const arc = knob.querySelector('.knob-arc');
            const indicator = knob.querySelector('.knob-indicator');
            const valueDisplay = knob.parentElement.querySelector('.knob-value');

            if (!track || !arc || !indicator || !valueDisplay) {
                console.error(`Knob ${index} (${paramName}): Missing SVG elements`);
                return;
            }

            function updateKnob(newValue, fromJuce = false) {
                value = Math.max(min, Math.min(max, newValue));
                const normalized = (value - min) / (max - min);

                // Draw track
                const trackPath = describeArc(35, 35, KNOB_RADIUS, ARC_START, ARC_START + ARC_RANGE);
                track.setAttribute('d', trackPath);

                // Draw arc
                const endAngle = ARC_START + (ARC_RANGE * normalized);
                const arcPath = describeArc(35, 35, KNOB_RADIUS, ARC_START, endAngle);
                arc.setAttribute('d', arcPath);

                // Position indicator
                const angle = (ARC_START + (ARC_RANGE * normalized)) * Math.PI / 180;
                const indicatorX = 35 + KNOB_RADIUS * Math.cos(angle) - 3.5;
                const indicatorY = 35 + KNOB_RADIUS * Math.sin(angle) - 3.5;
                indicator.style.left = `${indicatorX}px`;
                indicator.style.top = `${indicatorY}px`;

                // Update value display
                if (paramName === 'pitch') {
                    valueDisplay.textContent = value >= 0 ? `+${value.toFixed(2)}` : value.toFixed(2);
                } else {
                    valueDisplay.textContent = `${Math.round(normalized * 100)}%`;
                }

                // Update JUCE parameter (only if user interaction, not from JUCE)
                if (!fromJuce && parameterStates[paramName]) {
                    try {
                        parameterStates[paramName].setNormalisedValue(normalized);
                    } catch (error) {
                        console.warn(`Failed to update JUCE param ${paramName}:`, error.message);
                    }
                }
            }

            function describeArc(x, y, radius, startAngle, endAngle) {
                const start = polarToCartesian(x, y, radius, endAngle);
                const end = polarToCartesian(x, y, radius, startAngle);
                const largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
                return `M ${start.x} ${start.y} A ${radius} ${radius} 0 ${largeArcFlag} 0 ${end.x} ${end.y}`;
            }

            function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
                const angleInRadians = angleInDegrees * Math.PI / 180.0;
                return {
                    x: centerX + (radius * Math.cos(angleInRadians)),
                    y: centerY + (radius * Math.sin(angleInRadians))
                };
            }

            // Mouse drag handling
            knob.addEventListener('mousedown', (e) => {
                e.preventDefault();
                isDragging = true;
                startY = e.clientY;
                startValue = value;
                knob.classList.add('active');

                // Notify JUCE of drag start
                if (parameterStates[paramName]) {
                    try {
                        parameterStates[paramName].sliderDragStarted();
                    } catch (error) {
                        console.warn(`Failed to call sliderDragStarted for ${paramName}:`, error.message);
                    }
                }
            });

            document.addEventListener('mousemove', (e) => {
                if (isDragging) {
                    const deltaY = startY - e.clientY;
                    const sensitivity = 0.005;
                    const newValue = startValue + deltaY * (max - min) * sensitivity;
                    updateKnob(newValue, false);
                }
            });

            document.addEventListener('mouseup', () => {
                if (isDragging) {
                    isDragging = false;
                    knob.classList.remove('active');

                    // Notify JUCE of drag end
                    if (parameterStates[paramName]) {
                        try {
                            parameterStates[paramName].sliderDragEnded();
                        } catch (error) {
                            console.warn(`Failed to call sliderDragEnded for ${paramName}:`, error.message);
                        }
                    }
                }
            });

            // Double-click to reset
            knob.addEventListener('dblclick', () => {
                updateKnob(defaultValue, false);
            });

            // Initialize knob position
            updateKnob(defaultValue, false);

            // Listen for parameter changes from JUCE
            if (parameterStates[paramName]) {
                try {
                    // Use correct API: addValueChangedListener (not valueChangedEvent.addListener)
                    parameterStates[paramName].addValueChangedListener(() => {
                        const normalizedValue = parameterStates[paramName].getNormalisedValue();
                        const actualValue = min + (normalizedValue * (max - min));
                        updateKnob(actualValue, true);
                    });
                } catch (error) {
                    console.warn(`Failed to add listener for ${paramName}:`, error.message);
                }
            }

            console.log(`  ✓ Knob: ${paramName}`);
        } catch (error) {
            console.error(`  ✗ Knob ${index} failed:`, error);
        }
    });
}

function initializeModeTabs() {
    const modeTabs = document.querySelectorAll('.mode-tab');
    console.log(`Initializing ${modeTabs.length} mode tabs...`);

    modeTabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const mode = parseInt(tab.dataset.mode);

            // Update UI
            modeTabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            // Update JUCE parameter (normalize 0-3 to 0-1)
            if (parameterStates.mode) {
                try {
                    parameterStates.mode.setNormalisedValue(mode / 3.0);
                } catch (error) {
                    console.warn(`Failed to set mode: ${error.message}`);
                }
            }
        });
    });

    // Listen for mode changes from JUCE
    if (parameterStates.mode) {
        try {
            // Use correct API: addValueChangedListener (not valueChangedEvent.addListener)
            parameterStates.mode.addValueChangedListener(() => {
                const normalizedValue = parameterStates.mode.getNormalisedValue();
                const mode = Math.round(normalizedValue * 3.0);
                modeTabs.forEach(tab => {
                    tab.classList.toggle('active', parseInt(tab.dataset.mode) === mode);
                });
            });
        } catch (error) {
            console.warn(`Failed to add mode listener: ${error.message}`);
        }
    }

    console.log("  ✓ Mode tabs");
}

function initializeFreezeButton() {
    const freezeButton = document.getElementById('freezeButton');
    const freezeLED = document.getElementById('freezeLED');
    let freezeActive = false;

    console.log("Initializing freeze button...");

    freezeButton.addEventListener('click', () => {
        freezeActive = !freezeActive;

        // Update UI
        freezeButton.classList.toggle('active', freezeActive);
        freezeLED.classList.toggle('active', freezeActive);

        // Update JUCE parameter
        if (parameterStates.freeze) {
            try {
                parameterStates.freeze.setValue(freezeActive ? 1 : 0);
            } catch (error) {
                console.warn(`Failed to set freeze: ${error.message}`);
            }
        }
    });

    // Listen for freeze changes from JUCE
    if (parameterStates.freeze) {
        try {
            // Use correct API: addValueChangedListener (not valueChangedEvent.addListener)
            parameterStates.freeze.addValueChangedListener(() => {
                const active = parameterStates.freeze.getValue() > 0.5;
                freezeActive = active;
                freezeButton.classList.toggle('active', active);
                freezeLED.classList.toggle('active', active);
            });
        } catch (error) {
            console.warn(`Failed to add freeze listener: ${error.message}`);
        }
    }

    console.log("  ✓ Freeze button");
}

function initializeDropdowns() {
    const qualitySelect = document.getElementById('qualitySelect');
    const sampleModeSelect = document.getElementById('sampleModeSelect');

    console.log("Initializing dropdowns...");

    qualitySelect.addEventListener('change', (e) => {
        const value = parseInt(e.target.value);
        if (parameterStates.quality) {
            try {
                // Quality: 0-3 → normalize to 0-1
                parameterStates.quality.setNormalisedValue(value / 3.0);
            } catch (error) {
                console.warn(`Failed to set quality: ${error.message}`);
            }
        }
    });

    sampleModeSelect.addEventListener('change', (e) => {
        const value = parseInt(e.target.value);
        if (parameterStates.sample_mode) {
            try {
                // Sample mode: 0-1 → normalize to 0-1
                parameterStates.sample_mode.setNormalisedValue(value / 1.0);
            } catch (error) {
                console.warn(`Failed to set sample_mode: ${error.message}`);
            }
        }
    });

    // Listen for changes from JUCE
    if (parameterStates.quality) {
        try {
            // Use correct API: addValueChangedListener (not valueChangedEvent.addListener)
            parameterStates.quality.addValueChangedListener(() => {
                const normalizedValue = parameterStates.quality.getNormalisedValue();
                qualitySelect.value = Math.round(normalizedValue * 3.0);
            });
        } catch (error) {
            console.warn(`Failed to add quality listener: ${error.message}`);
        }
    }

    if (parameterStates.sample_mode) {
        try {
            // Use correct API: addValueChangedListener (not valueChangedEvent.addListener)
            parameterStates.sample_mode.addValueChangedListener(() => {
                const normalizedValue = parameterStates.sample_mode.getNormalisedValue();
                sampleModeSelect.value = Math.round(normalizedValue * 1.0);
            });
        } catch (error) {
            console.warn(`Failed to add sample_mode listener: ${error.message}`);
        }
    }

    console.log("  ✓ Dropdowns");
}

function initializeMeters() {
    const inputMeter = document.getElementById('inputMeter');
    const outputMeter = document.getElementById('outputMeter');

    console.log("Initializing meters...");

    // Create meter segments
    function createMeterSegments(meter) {
        for (let i = 0; i < 24; i++) {
            const segment = document.createElement('div');
            segment.className = 'meter-segment';
            meter.appendChild(segment);
        }
    }

    createMeterSegments(inputMeter);
    createMeterSegments(outputMeter);

    // Meter update function
    function updateMeter(meter, level) {
        const segments = meter.querySelectorAll('.meter-segment');
        const activeSegments = Math.floor(level * 24);

        segments.forEach((segment, index) => {
            segment.classList.remove('active', 'green', 'yellow', 'red');
            if (index < activeSegments) {
                segment.classList.add('active');
                if (index < 19) segment.classList.add('green');
                else if (index < 23) segment.classList.add('yellow');
                else segment.classList.add('red');
            }
        });
    }

    // Animation loop (demo - will be replaced with DSP values)
    function animateMeters() {
        updateMeter(inputMeter, 0.3 + Math.random() * 0.4);
        updateMeter(outputMeter, 0.4 + Math.random() * 0.3);
        requestAnimationFrame(animateMeters);
    }

    animateMeters();
    console.log("  ✓ Meters");
}

function initializeGrainVisualization() {
    const canvas = document.getElementById('grainCanvas');
    if (!canvas) {
        console.warn("Grain canvas not found");
        return;
    }

    const ctx = canvas.getContext('2d');
    canvas.width = 680;
    canvas.height = 60;

    console.log("Initializing grain visualization...");

    // Particle system
    const particles = [];
    for (let i = 0; i < 35; i++) {
        particles.push({
            x: Math.random() * canvas.width,
            y: Math.random() * canvas.height,
            vx: (Math.random() - 0.5) * 0.5,
            vy: (Math.random() - 0.5) * 0.3,
            size: 2 + Math.random() * 3,
            opacity: 0.3 + Math.random() * 0.3
        });
    }

    function animateGrains() {
        // Fade effect
        ctx.fillStyle = 'rgba(26, 26, 46, 0.1)';
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Get freeze state (safely)
        let isFrozen = false;
        try {
            isFrozen = parameterStates.freeze && parameterStates.freeze.getValue() > 0.5;
        } catch (error) {
            // Ignore - freeze not available
        }

        particles.forEach(p => {
            // Update position
            if (!isFrozen) {
                p.x += p.vx;
                p.y += p.vy;

                // Bounce off edges
                if (p.x < 0 || p.x > canvas.width) p.vx *= -1;
                if (p.y < 0 || p.y > canvas.height) p.vy *= -1;
            }

            // Render particle
            const tint = isFrozen ? '180, 200, 210' : '90, 200, 216';
            ctx.fillStyle = `rgba(${tint}, ${isFrozen ? p.opacity * 0.3 : p.opacity})`;
            ctx.beginPath();
            ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
            ctx.fill();
        });

        requestAnimationFrame(animateGrains);
    }

    animateGrains();
    console.log("  ✓ Grain visualization");
}

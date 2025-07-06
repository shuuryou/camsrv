/**
 * Camera Viewer Navigation
 * Handles recording selection and navigation between camera recordings
 */
class CameraViewer {
    constructor() {
        this.dateSelect = null;
        this.previousBtn = null;
        this.nextBtn = null;
        this.initialize();
    }

    /**
     * Initialize the viewer when DOM is ready
     */
    initialize() {
        // Use modern DOM ready check
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }

    /**
     * Setup DOM elements and event handlers
     */
    setup() {
        // Get DOM elements
        this.dateSelect = document.getElementById('date');
        this.previousBtn = document.getElementById('previous');
        this.nextBtn = document.getElementById('next');

        // Check if all required elements exist
        if (!this.dateSelect) {
            console.warn('Date selector not found');
            return;
        }

        // Setup event listeners
        this.setupEventListeners();

        // Update button states
        this.updateButtonStates();
    }

    /**
     * Setup event listeners for navigation
     */
    setupEventListeners() {
        // Add keyboard navigation
        document.addEventListener('keydown', (e) => {
            if (e.key === 'ArrowLeft' && this.previousBtn && !this.previousBtn.disabled) {
                this.navigatePrevious();
            } else if (e.key === 'ArrowRight' && this.nextBtn && !this.nextBtn.disabled) {
                this.navigateNext();
            }
        });

        this.previousBtn.addEventListener('click', () => {
            this.navigatePrevious(this.getCurrentCamera());
        });

        this.nextBtn.addEventListener('click', () => {
            this.navigateNext(this.getCurrentCamera());
        });

        // Add change event to date selector
        if (this.dateSelect) {
            this.dateSelect.addEventListener('change', (e) => {
                const camera = this.getCurrentCamera();
                if (camera) {
                    this.navigateToRecording(camera, e.target.value);
                }
            });
        }
    }

    /**
     * Navigate to previous recording
     * @param {string} camera - Camera identifier
     */
    navigatePrevious(camera = null) {
        if (!this.dateSelect || this.dateSelect.selectedIndex >= this.dateSelect.length - 1) {
            return;
        }

        this.dateSelect.selectedIndex += 1;
        this.navigateToRecording(camera || this.getCurrentCamera(), this.dateSelect.value);
    }

    /**
     * Navigate to next recording
     * @param {string} camera - Camera identifier
     */
    navigateNext(camera = null) {
        if (!this.dateSelect || this.dateSelect.selectedIndex <= 0) {
            return;
        }

        this.dateSelect.selectedIndex -= 1;
        this.navigateToRecording(camera || this.getCurrentCamera(), this.dateSelect.value);
    }

    /**
     * Navigate to specific recording
     * @param {string} camera - Camera identifier
     * @param {string} recordingId - Recording identifier
     */
    navigateToRecording(camera, recordingId) {
        const params = new URLSearchParams();

        if (camera) {
            params.append('camera', camera);
        }

        if (recordingId) {
            params.append('recording', recordingId);
        }

        window.location.href = `view.php?${params.toString()}`;
    }

    /**
     * Update navigation button states based on current selection
     */
    updateButtonStates() {
        if (!this.dateSelect || !this.previousBtn || !this.nextBtn) {
            return;
        }

        // Disable previous button if at the last item (newest recording)
        this.previousBtn.disabled = this.dateSelect.selectedIndex >= this.dateSelect.length - 1;

        // Disable next button if at the first item (oldest recording)
        this.nextBtn.disabled = this.dateSelect.selectedIndex <= 0;
    }

    /**
     * Get current camera from URL or page context
     * @returns {string|null} Camera identifier
     */
    getCurrentCamera() {
        // Try to get from URL first
        const urlParams = new URLSearchParams(window.location.search);
        const camera = urlParams.get('camera');

        if (camera) {
            return camera;
        }

        // Fallback: try to get from a data attribute or hidden input
        const cameraElement = document.querySelector('[data-camera]');
        return cameraElement ? cameraElement.dataset.camera : null;
    }

    /**
     * Public method to go to a specific recording (legacy support)
     * @param {string} camera - Camera identifier
     * @param {HTMLSelectElement} selectElement - Date selector element
     */
    go(camera, selectElement) {
        if (!this.dateSelect && selectElement) {
            this.dateSelect = selectElement;
        }

        if (this.dateSelect) {
            this.navigateToRecording(camera, this.dateSelect.value);
        }
    }
}

const cameraViewer = new CameraViewer();
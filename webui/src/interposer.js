class ControlInterposer {
  /**
   * Creates a new interposer instance
   * @param {HTMLElement|null} outputElement - The element to update with control changes
   */
  constructor(outputElement = null) {
    this.outputElement = outputElement;
    this.enableUIUpdates = true;
    this.enableConsoleLogging = true;
    this.enableJUCE = true;  // ok to leave this enabled even when developing; we check for window.__JUCE__
    this.currentDrum = 'drum1'; // Track the currently active drum
  }

  /**
   * Sets the output element
   * @param {HTMLElement} element - The element to use for UI updates
   */
  setOutputElement(element) {
    this.outputElement = element;
  }

  /**
   * Toggles UI updates on or off
   * @param {boolean} enabled - Whether UI updates should be enabled
   */
  setUIUpdates(enabled) {
    this.enableUIUpdates = Boolean(enabled);
  }

  /**
   * Toggles console logging on or off
   * @param {boolean} enabled - Whether console logging should be enabled
   */
  setConsoleLogging(enabled) {
    this.enableConsoleLogging = Boolean(enabled);
  }

  setJUCEEnabled(enabled) {
    this.enableJUCE = Boolean(enabled);
  }

  /**
   * Track which drum is currently active
   * @param {string} drumId - The ID of the active drum
   */
  setActiveDrum(drumId) {
    this.currentDrum = drumId;
    if (this.enableConsoleLogging) {
      console.log(`Active drum changed: ${drumId}`);
    }
  }

  /**
   * Handles a control change by updating UI and/or logging to console based on settings
   * @param {string} id - The ID of the control that changed
   * @param {number|string|boolean} value - The new value of the control
   */
  handleControlChange(id, value) {
    let message = '';
    
    // Check if this is a drum-specific control
    if (id.includes(':')) {
      const [drumId, controlId] = id.split(':');
      message = `${drumId} - ${controlId}: ${value}`;
      
      // Update the active drum if this is a drum change event
      if (id === 'active-drum') {
        this.setActiveDrum(value.split(' ')[0]);
      }

      if (this.enableJUCE)  {
        if (window.__JUCE__) {
          window.__JUCE__.backend.emitEvent("controlChange", {
            drum: drumId,
            control: controlId,
            value: value
          });
          // console.log(`JUCE event emitted for control: ${controlId}`);
        }
      }
      // end drum-specific control
    } else {
      if (this.enableJUCE && window.__JUCE__) {
        window.__JUCE__.backend.emitEvent("genericEvent", {
          id: id,
          value: value
        });
      }
      message = `${id}: ${value}`;
    }
    
    // Update UI if enabled and we have an output element
    if (this.enableUIUpdates && this.outputElement) {
      this.outputElement.textContent = message;
    }
    
    // Log to console if enabled
    if (this.enableConsoleLogging) {
      console.log(`Control changed: ${message}`);
    }
  }
}

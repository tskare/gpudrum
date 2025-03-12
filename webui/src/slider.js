class RotaryKnob extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this._value = 5; // Default value (middle position)
    this._min = 0;
    this._max = 10;
    this._dragging = false;
    this._startY = 0;
    this._startValue = 0;
    this._sensitivity = 0.01; // Sensitivity of drag movement
  }

  static get observedAttributes() {
    return ['value', 'min', 'max'];
  }

  connectedCallback() {
    // Create the SVG knob
    this.render();
    
    // Add event listeners for interaction
    this.shadowRoot.addEventListener('mousedown', this.handleMouseDown.bind(this));
    window.addEventListener('mousemove', this.handleMouseMove.bind(this));
    window.addEventListener('mouseup', this.handleMouseUp.bind(this));
    
    // Touch events
    this.shadowRoot.addEventListener('touchstart', this.handleTouchStart.bind(this));
    window.addEventListener('touchmove', this.handleTouchMove.bind(this));
    window.addEventListener('touchend', this.handleTouchEnd.bind(this));

    this.valueDisplay = this.shadowRoot.querySelector('.value-display');
    this.valueDisplay.style.visibility = 'hidden';
  }

  attributeChangedCallback(name, oldValue, newValue) {
    if (name === 'value') {
      this._value = parseFloat(newValue) || 0;
      this.updateKnobPosition();
    } else if (name === 'min') {
      this._min = parseFloat(newValue) || 0;
    } else if (name === 'max') {
      this._max = parseFloat(newValue) || 10;
    }
  }

render() {
    const size = 100;
    const center = size / 2;
    const radius = size * 0.4;
    
    this.shadowRoot.innerHTML = `
        <style>
            :host {
                display: inline-block;
                width: 50px;  /* Changed from 100px to 50px */
                height: 60px;  /* Changed from 120px to 60px */
                user-select: none;
            }
            .knob-container {
                position: relative;
                width: 100%;
                height: 100%;
                display: flex;
                flex-direction: column;
                align-items: center;
            }
            svg {
                width: 50px;  /* Changed from 100px to 50px */
                height: 50px;  /* Changed from 100px to 50px */
                cursor: grab;
            }
            svg.dragging {
                cursor: grabbing;
            }
            .knob-body {
                fill: #2a2a2a;
                stroke: #444;
                stroke-width: 2px;
            }
            .knob-indicator {
                fill: #ddd;
                stroke: none;
            }
            .knob-dot {
                fill: #f5f5f5;
                stroke: none;
            }
            .indicator-ring {
                fill: none;
                stroke: #444;
                stroke-width: 4px;
                stroke-dasharray: 565;
                stroke-dashoffset: 142;
            }
            .active-indicator {
                fill: none;
                stroke: #f26d21;
                stroke-width: 4px;
                stroke-linecap: round;
            }
            .value-display {
                font-family: sans-serif;
                font-size: 12px;  /* Changed from 14px to 12px */
                font-weight: bold;
                margin-top: 3px;  /* Changed from 5px to 3px */
                color: #ddd;
            }
        </style>
        <div class="knob-container">
            <svg viewBox="0 0 ${size} ${size}">
                <circle class="indicator-ring" cx="${center}" cy="${center}" r="${radius + 10}" />
                <path class="active-indicator" id="active-path" />
                <circle class="knob-body" cx="${center}" cy="${center}" r="${radius}" />
                <rect class="knob-indicator" id="indicator" 
                            x="${center - 2}" y="${center - radius + 5}" 
                            width="4" height="${radius - 10}" 
                            rx="2" ry="2" />
                <!-- <circle class="knob-dot" cx="${center}" cy="${center}" r="3" /> -->
            </svg>
            <div class="value-display">${this._value}</div>
        </div>
    `;
    
    this.updateKnobPosition();
}

updateKnobPosition() {
    if (!this.shadowRoot) return;
    
    const angle = this.valueToAngle(this._value);
    const svg = this.shadowRoot.querySelector('svg');
    const indicator = this.shadowRoot.querySelector('#indicator');
    const display = this.shadowRoot.querySelector('.value-display');
    const activePath = this.shadowRoot.querySelector('#active-path');
    
    const size = 100;
    const center = size / 2;
    const radius = size * 0.4;
    
    // Update the indicator rotation
    indicator.setAttribute('transform', `rotate(${angle}, ${center}, ${center})`);
    
    // Update the display
    display.textContent = this._value.toFixed(1);
    
    // Update the active arc
    const startAngle = -135 - 90; // Shift start angle 90 degrees counterclockwise
    const endAngle = angle - 90;  // Also shift end angle by 90 degrees
    
    const largeArcFlag = endAngle - startAngle > 180 ? 1 : 0;
    
    // Calculate points on the arc
    const arcRadius = radius + 10;
    const x1 = center + arcRadius * Math.cos((startAngle * Math.PI) / 180);
    const y1 = center + arcRadius * Math.sin((startAngle * Math.PI) / 180);
    const x2 = center + arcRadius * Math.cos((endAngle * Math.PI) / 180);
    const y2 = center + arcRadius * Math.sin((endAngle * Math.PI) / 180);
    
    activePath.setAttribute(
      'd',
      `M ${x1},${y1} A ${arcRadius},${arcRadius} 0 ${largeArcFlag} 1 ${x2},${y2}`
    );
}

  valueToAngle(value) {
    const min = this._min;
    const max = this._max;
    // Map value to angle (from -135 to 135 degrees)
    return -135 + ((value - min) / (max - min)) * 270;
  }

  angleToValue(angle) {
    const min = this._min;
    const max = this._max;
    // Map angle (from -135 to 135) to value
    return min + ((angle + 135) / 270) * (max - min);
  }

  handleMouseDown(event) {
    this._dragging = true;
    this._startY = event.clientY;
    this._startValue = this._value;
    this.shadowRoot.querySelector('svg').classList.add('dragging');
    this.valueDisplay.style.visibility = 'visible';
    event.preventDefault();
  }

  handleMouseMove(event) {
    if (!this._dragging) return;
    
    const deltaY = this._startY - event.clientY;
    const deltaValue = deltaY * this._sensitivity * (this._max - this._min);
    
    let newValue = this._startValue + deltaValue;
    
    // Clamp and set value
    newValue = Math.max(this._min, Math.min(this._max, newValue));
    this._value = newValue;
    this.updateKnobPosition();
    
    // Notify listeners.
    this.dispatchEvent(new CustomEvent('change', { 
      detail: { value: this._value },
      bubbles: true
    }));
  }

  handleMouseUp() {
    if (this._dragging) {
      this._dragging = false;
      this.shadowRoot.querySelector('svg')?.classList.remove('dragging');
      this.valueDisplay.style.visibility = 'hidden';
    }
  }

  // Touch event handlers for mobile devices.
  // TODO(travis): Test on a real device; only checked in devtools so far.
  handleTouchStart(event) {
    this._dragging = true;
    this._startY = event.touches[0].clientY;
    this._startValue = this._value;
    this.shadowRoot.querySelector('svg').classList.add('dragging');
    this.valueDisplay.style.visibility = 'visible';
    event.preventDefault();
  }

  handleTouchMove(event) {
    if (!this._dragging) return;
    
    const deltaY = this._startY - event.touches[0].clientY;
    const deltaValue = deltaY * this._sensitivity * (this._max - this._min);
    
    let newValue = this._startValue + deltaValue;
    newValue = Math.max(this._min, Math.min(this._max, newValue));
    
    this._value = newValue;
    this.updateKnobPosition();
    
    this.dispatchEvent(new CustomEvent('change', { 
      detail: { value: this._value },
      bubbles: true
    }));
  }

  handleTouchEnd() {
    if (this._dragging) {
      this._dragging = false;
      this.shadowRoot.querySelector('svg')?.classList.remove('dragging');
      this.valueDisplay.style.visibility = 'hidden';
    }
  }

  // Public API
  get value() {
    return this._value;
  }
  
  set value(val) {
    const newValue = Math.max(this._min, Math.min(this._max, val));
    this._value = newValue;
    this.updateKnobPosition();
  }
}

// Register the custom element
customElements.define('rotary-knob', RotaryKnob);
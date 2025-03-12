class DrumPanel extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    // Initialize data for multiple drums and cymbals
    this.currentDrum = 'drum1'; // Default selected drum
    this.drumsData = {
      // Drums
      'drum1': { name: 'Kick', type: 'yamaha10', values: {} },
      'drum2': { name: 'Snare', type: 'dw12', values: {} },
      'drum3': { name: 'Tom 1', type: 'pearl14', values: {} },
      'drum4': { name: 'Tom 2', type: 'ludwig13', values: {} },
      'drum5': { name: 'Tom 3', type: 'pearl14', values: {} },
      'drum6': { name: 'Floor Tom', type: 'ludwig13', values: {} },
      // Cymbals
      'cymbal1': { name: 'Hi-Hat', type: 'yamaha10', values: {} },
      'cymbal2': { name: 'Crash 1', type: 'dw12', values: {} },
      'cymbal3': { name: 'Crash 2', type: 'pearl14', values: {} },
      'cymbal4': { name: 'Ride', type: 'ludwig13', values: {} },
      'cymbal5': { name: 'Splash', type: 'yamaha10', values: {} },
      'cymbal6': { name: 'China', type: 'dw12', values: {} }
    };
    
    // Initialize default values for all knobs for all drums
    const knobIds = [
      'pitch-knob', 'decay-knob', 'attack-knob', 'tone-knob',
      'ring-knob', 'tension-knob', 'size-knob', 'density-knob',
      'damping-knob', 'velocity-knob', 'volume-knob', 'pan-knob'
    ];
    
    Object.keys(this.drumsData).forEach(drumId => {
      knobIds.forEach(knobId => {
        this.drumsData[drumId].values[knobId] = 5; // Default value
      });
      // Default checkbox values
      this.drumsData[drumId].values['mute-checkbox'] = false;
      this.drumsData[drumId].values['solo-checkbox'] = false;
    });
  }

  connectedCallback() {
    this.render();
    this.setupEventListeners();
  }

  render() {
    const currentDrumData = this.drumsData[this.currentDrum];
    
    this.shadowRoot.innerHTML = `
      <link rel="stylesheet" href="main.css">
      <div id="container" class="drum-panel">
        <div class="panel-content-vertical">
          <!-- Section Label -->
          <div class="panel-header">
            <h2 class="panel-title">${currentDrumData.name}</h2>
          </div>
          
          <!-- Drum Selection Dropdown -->
          <div class="control-wrapper">
            <div class="control-label">Drum Type</div>
            <select id="drum-select" class="drum-select">
            <!-- Drum options, limited demo --> 
            
              <option value="yamaha10" ${currentDrumData.type === 'yamaha10' ? 'selected' : ''}>10" Yamaha</option>
              <option value="dw12" ${currentDrumData.type === 'dw12' ? 'selected' : ''}>12" DW Maple</option>
              <option value="pearl14" ${currentDrumData.type === 'pearl14' ? 'selected' : ''}>14" Pearl Masters</option>
              <option value="ludwig13" ${currentDrumData.type === 'ludwig13' ? 'selected' : ''}>13" Ludwig Classic</option>
              
            <!-- Full v1 mode sets -->
<option value="10_zild_a_custom_splash" ${currentDrumData.type === '10_zild_a_custom_splash' ? 'selected' : ''}>10" Zildjian A Custom Splash</option>
<option value="13_sab_dejonte_crash" ${currentDrumData.type === '13_sab_dejonte_crash' ? 'selected' : ''}>13" Sabian Dejonte Splash</option>
<option value="13_sab_elsabor_splash" ${currentDrumData.type === '13_sab_elsabor_splash' ? 'selected' : ''}>13" Sabian El Sabor Splash</option>
<option value="14_zild_1960s_vintage" ${currentDrumData.type === '14_zild_1960s_vintage' ? 'selected' : ''}>14" Zildjian 1960s</option>
<option value="16_paiste_twenty_thin" ${currentDrumData.type === '16_paiste_twenty_thin' ? 'selected' : ''}>16" Paiste 2020 Thin</option>
<option value="16_sab_hhxtreme" ${currentDrumData.type === '16_sab_hhxtreme' ? 'selected' : ''}>16" Sabian HHXTreme</option>
<option value="16_sab_hhx_evo" ${currentDrumData.type === '16_sab_hhx_evo' ? 'selected' : ''}>16" Sabian HHX Evo</option>
<option value="16_sab_hhx_evo_alternate_recording" ${currentDrumData.type === '16_sab_hhx_evo_alternate_recording' ? 'selected' : ''}>16" Sabian hhx evo (alt)</option>
<option value="16_sab_hhx_ozone" ${currentDrumData.type === '16_sab_hhx_ozone' ? 'selected' : ''}>16_sab_hhx_ozone</option>
<option value="16_zild_1960s_vintage" ${currentDrumData.type === '16_zild_1960s_vintage' ? 'selected' : ''}>16_zild_1960s_vintage</option>
<option value="16_zild_avedis_vintage" ${currentDrumData.type === '16_zild_avedis_vintage' ? 'selected' : ''}>16_zild_avedis_vintage</option>
<option value="16_zild_k_dark" ${currentDrumData.type === '16_zild_k_dark' ? 'selected' : ''}>16_zild_k_dark</option>
<option value="17_hhx_evo" ${currentDrumData.type === '17_hhx_evo' ? 'selected' : ''}>17_hhx_evo</option>
<option value="18_paiste_2002_black_vintage" ${currentDrumData.type === '18_paiste_2002_black_vintage' ? 'selected' : ''}>18_paiste_2002_black_vintage</option>
<option value="18_sab_hhx" ${currentDrumData.type === '18_sab_hhx' ? 'selected' : ''}>18_sab_hhx</option>
<option value="18_sab_hhxtreme" ${currentDrumData.type === '18_sab_hhxtreme' ? 'selected' : ''}>18_sab_hhxtreme</option>
<option value="18_sab_hhx_1" ${currentDrumData.type === '18_sab_hhx_1' ? 'selected' : ''}>18_sab_hhx_1</option>
<option value="18_sab_hhx_ozone" ${currentDrumData.type === '18_sab_hhx_ozone' ? 'selected' : ''}>18_sab_hhx_ozone</option>
<option value="18_zild_avedis_vintage" ${currentDrumData.type === '18_zild_avedis_vintage' ? 'selected' : ''}>18_zild_avedis_vintage</option>
<option value="18_zild_a_custom" ${currentDrumData.type === '18_zild_a_custom' ? 'selected' : ''}>18_zild_a_custom</option>
<option value="18_zild_k_dark" ${currentDrumData.type === '18_zild_k_dark' ? 'selected' : ''}>18_zild_k_dark</option>
<option value="18_zild_k_med_dark" ${currentDrumData.type === '18_zild_k_med_dark' ? 'selected' : ''}>18_zild_k_med_dark</option>
<option value="18_zild_k_med_dark_crash" ${currentDrumData.type === '18_zild_k_med_dark_crash' ? 'selected' : ''}>18_zild_k_med_dark_crash</option>
<option value="19_sab_aa_medthin" ${currentDrumData.type === '19_sab_aa_medthin' ? 'selected' : ''}>19_sab_aa_medthin</option>
<option value="19_sab_mediumthin" ${currentDrumData.type === '19_sab_mediumthin' ? 'selected' : ''}>19_sab_mediumthin</option>
<option value="19_sab_paragon_CHINA" ${currentDrumData.type === '19_sab_paragon_CHINA' ? 'selected' : ''}>19_sab_paragon_CHINA</option>
<option value="19_zild_1960s_vintage" ${currentDrumData.type === '19_zild_1960s_vintage' ? 'selected' : ''}>19_zild_1960s_vintage</option>
<option value="19_zild_a_custom_projection" ${currentDrumData.type === '19_zild_a_custom_projection' ? 'selected' : ''}>19_zild_a_custom_projection</option>
<option value="19_zild_k_const" ${currentDrumData.type === '19_zild_k_const' ? 'selected' : ''}>19_zild_k_const</option>
<option value="19_zild_k_cust_hybrid" ${currentDrumData.type === '19_zild_k_cust_hybrid' ? 'selected' : ''}>19_zild_k_cust_hybrid</option>
<option value="19_zild_k_c_hybrid_CHINA" ${currentDrumData.type === '19_zild_k_c_hybrid_CHINA' ? 'selected' : ''}>19_zild_k_c_hybrid_CHINA</option>
<option value="20_paiste_2002_black_vintage" ${currentDrumData.type === '20_paiste_2002_black_vintage' ? 'selected' : ''}>20_paiste_2002_black_vintage</option>
<option value="20_sab_hhx_ride" ${currentDrumData.type === '20_sab_hhx_ride' ? 'selected' : ''}>20_sab_hhx_ride</option>
<option value="20_sab_vault_crash" ${currentDrumData.type === '20_sab_vault_crash' ? 'selected' : ''}>20_sab_vault_crash</option>
<option value="20_zild_avedis_vintage" ${currentDrumData.type === '20_zild_avedis_vintage' ? 'selected' : ''}>20_zild_avedis_vintage</option>
<option value="20_zild_a_custom" ${currentDrumData.type === '20_zild_a_custom' ? 'selected' : ''}>20_zild_a_custom</option>
<option value="20_zild_oriental_crash_doom" ${currentDrumData.type === '20_zild_oriental_crash_doom' ? 'selected' : ''}>20_zild_oriental_crash_doom</option>
<option value="21_sab_aaxplosion" ${currentDrumData.type === '21_sab_aaxplosion' ? 'selected' : ''}>21_sab_aaxplosion</option>
<option value="22_zild_avedis_crash_ride" ${currentDrumData.type === '22_zild_avedis_crash_ride' ? 'selected' : ''}>22_zild_avedis_crash_ride</option>
<option value="cowbell_BB" ${currentDrumData.type === 'cowbell_BB' ? 'selected' : ''}>cowbell_BB</option>
<option value="cowbell_LP" ${currentDrumData.type === 'cowbell_LP' ? 'selected' : ''}>cowbell_LP</option>
<option value="cowbell_MC" ${currentDrumData.type === 'cowbell_MC' ? 'selected' : ''}>cowbell_MC</option>
<option value="kick22pearlmasters" ${currentDrumData.type === 'kick22pearlmasters' ? 'selected' : ''}>kick22pearlmasters</option>
<option value="kick22yamahabirch" ${currentDrumData.type === 'kick22yamahabirch' ? 'selected' : ''}>kick22yamahabirch</option>
<option value="kick24dwcollmaple" ${currentDrumData.type === 'kick24dwcollmaple' ? 'selected' : ''}>kick24dwcollmaple</option>
<option value="kick24ludwigvintage" ${currentDrumData.type === 'kick24ludwigvintage' ? 'selected' : ''}>kick24ludwigvintage</option>
<option value="snrBlackPanther" ${currentDrumData.type === 'snrBlackPanther' ? 'selected' : ''}>snrBlackPanther</option>
<option value="snrDWCopper" ${currentDrumData.type === 'snrDWCopper' ? 'selected' : ''}>snrDWCopper</option>
<option value="snrJarrahBlack" ${currentDrumData.type === 'snrJarrahBlack' ? 'selected' : ''}>snrJarrahBlack</option>
<option value="snrLudwigBB100thAnniv" ${currentDrumData.type === 'snrLudwigBB100thAnniv' ? 'selected' : ''}>snrLudwigBB100thAnniv</option>
<option value="snrLudwigBlackBeauty" ${currentDrumData.type === 'snrLudwigBlackBeauty' ? 'selected' : ''}>snrLudwigBlackBeauty</option>
<option value="snrLudwigBlackMagic" ${currentDrumData.type === 'snrLudwigBlackMagic' ? 'selected' : ''}>snrLudwigBlackMagic</option>
<option value="snrLudwigMahogany" ${currentDrumData.type === 'snrLudwigMahogany' ? 'selected' : ''}>snrLudwigMahogany</option>
<option value="snrLudwigSensitive1929" ${currentDrumData.type === 'snrLudwigSensitive1929' ? 'selected' : ''}>snrLudwigSensitive1929</option>
<option value="snrMetroJarrah" ${currentDrumData.type === 'snrMetroJarrah' ? 'selected' : ''}>snrMetroJarrah</option>
<option value="snrOcheltreeCastCarbonSteel" ${currentDrumData.type === 'snrOcheltreeCastCarbonSteel' ? 'selected' : ''}>snrOcheltreeCastCarbonSteel</option>
<option value="snrPearlMahogany" ${currentDrumData.type === 'snrPearlMahogany' ? 'selected' : ''}>snrPearlMahogany</option>
<option value="snrPearlSensitoneBrass" ${currentDrumData.type === 'snrPearlSensitoneBrass' ? 'selected' : ''}>snrPearlSensitoneBrass</option>
<option value="snrPremiereVintage" ${currentDrumData.type === 'snrPremiereVintage' ? 'selected' : ''}>snrPremiereVintage</option>
<option value="snrQCopper" ${currentDrumData.type === 'snrQCopper' ? 'selected' : ''}>snrQCopper</option>
<option value="snrSonorDesigner" ${currentDrumData.type === 'snrSonorDesigner' ? 'selected' : ''}>snrSonorDesigner</option>
<option value="snrZildjianCooley" ${currentDrumData.type === 'snrZildjianCooley' ? 'selected' : ''}>snrZildjianCooley</option>
<option value="tom10dwcoll" ${currentDrumData.type === 'tom10dwcoll' ? 'selected' : ''}>tom10dwcoll</option>
<option value="tom10pearlmasters" ${currentDrumData.type === 'tom10pearlmasters' ? 'selected' : ''}>tom10pearlmasters</option>
<option value="tom10yamahabirchabs" ${currentDrumData.type === 'tom10yamahabirchabs' ? 'selected' : ''}>tom10yamahabirchabs</option>
<option value="tom12dwcoll" ${currentDrumData.type === 'tom12dwcoll' ? 'selected' : ''}>tom12dwcoll</option>
<option value="tom12pearlmastersmaple" ${currentDrumData.type === 'tom12pearlmastersmaple' ? 'selected' : ''}>tom12pearlmastersmaple</option>
<option value="tom12rogersbigr" ${currentDrumData.type === 'tom12rogersbigr' ? 'selected' : ''}>tom12rogersbigr</option>
<option value="tom12yamahabirchabs" ${currentDrumData.type === 'tom12yamahabirchabs' ? 'selected' : ''}>tom12yamahabirchabs</option>
<option value="tom13rogersbigr" ${currentDrumData.type === 'tom13rogersbigr' ? 'selected' : ''}>tom13rogersbigr</option>
<option value="tom14yamahabirchabs" ${currentDrumData.type === 'tom14yamahabirchabs' ? 'selected' : ''}>tom14yamahabirchabs</option>
<option value="tom16dwcustom" ${currentDrumData.type === 'tom16dwcustom' ? 'selected' : ''}>tom16dwcustom</option>
<option value="tom16pearlmastersmaple" ${currentDrumData.type === 'tom16pearlmastersmaple' ? 'selected' : ''}>tom16pearlmastersmaple</option>
<option value="tom16rogersbigr" ${currentDrumData.type === 'tom16rogersbigr' ? 'selected' : ''}>tom16rogersbigr</option>
            </select>
          </div>
          
          <!-- Rotary Knob Controls - 3 rows of 4 knobs -->
          <div class="knobs-grid">
            <!-- Row 1 -->
            <div class="knob-row">
              <div class="knob-wrapper">
                <div class="control-label">Pitch</div>
                <rotary-knob id="pitch-knob" value="${currentDrumData.values['pitch-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Decay</div>
                <rotary-knob id="decay-knob" value="${currentDrumData.values['decay-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Attack</div>
                <rotary-knob id="attack-knob" value="${currentDrumData.values['attack-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Tone</div>
                <rotary-knob id="tone-knob" value="${currentDrumData.values['tone-knob'] || 5}"></rotary-knob>
              </div>
            </div>
            
            <!-- Row 2 -->
            <div class="knob-row">
              <div class="knob-wrapper">
                <div class="control-label">Ring</div>
                <rotary-knob id="ring-knob" value="${currentDrumData.values['ring-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Tension</div>
                <rotary-knob id="tension-knob" value="${currentDrumData.values['tension-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Size</div>
                <rotary-knob id="size-knob" value="${currentDrumData.values['size-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Density</div>
                <rotary-knob id="density-knob" value="${currentDrumData.values['density-knob'] || 5}"></rotary-knob>
              </div>
            </div>
            
            <!-- Row 3 -->
            <div class="knob-row">
              <div class="knob-wrapper">
                <div class="control-label">Damping</div>
                <rotary-knob id="damping-knob" value="${currentDrumData.values['damping-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Velocity</div>
                <rotary-knob id="velocity-knob" value="${currentDrumData.values['velocity-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Volume</div>
                <rotary-knob id="volume-knob" value="${currentDrumData.values['volume-knob'] || 5}"></rotary-knob>
              </div>
              <div class="knob-wrapper">
                <div class="control-label">Pan</div>
                <rotary-knob id="pan-knob" value="${currentDrumData.values['pan-knob'] || 5}"></rotary-knob>
              </div>
            </div>
          </div>
          
          <!-- Mute and Solo Checkboxes -->
          <div class="checkbox-controls">
            <div class="checkbox-wrapper">
              <input type="checkbox" id="mute-checkbox" class="control-checkbox" ${currentDrumData.values['mute-checkbox'] ? 'checked' : ''}>
              <label for="mute-checkbox" class="control-label">Mute</label>
            </div>
            <div class="checkbox-wrapper">
              <input type="checkbox" id="solo-checkbox" class="control-checkbox" ${currentDrumData.values['solo-checkbox'] ? 'checked' : ''}>
              <label for="solo-checkbox" class="control-label">Solo</label>
            </div>
          </div>
        </div>
      </div>
    `;
  }

  setupEventListeners() {
    // Handle knob changes
    this.shadowRoot.querySelectorAll('rotary-knob').forEach(knob => {
      knob.addEventListener('change', (e) => {
        const id = knob.id;
        const value = e.detail.value.toFixed(1);
        
        // Save the value for the current drum
        this.drumsData[this.currentDrum].values[id] = parseFloat(value);
        
        // Dispatch a custom event that the parent can listen to
        this.dispatchEvent(new CustomEvent('knob-change', {
          bubbles: true,
          detail: { 
            id, 
            value,
            drum: this.currentDrum 
          }
        }));
      });
    });

    // Handle checkbox changes
    this.shadowRoot.querySelectorAll('.control-checkbox').forEach(checkbox => {
      checkbox.addEventListener('change', (e) => {
        const id = checkbox.id;
        const value = checkbox.checked;
        
        // Save the value for the current drum
        this.drumsData[this.currentDrum].values[id] = value;
        
        // Dispatch event
        this.dispatchEvent(new CustomEvent('checkbox-change', {
          bubbles: true,
          detail: { 
            id, 
            value,
            drum: this.currentDrum 
          }
        }));
      });
    });
    
    // Handle drum type selection
    this.shadowRoot.querySelector('#drum-select').addEventListener('change', (e) => {
      const value = e.target.value;
      
      // Save the selected type for the current drum
      this.drumsData[this.currentDrum].type = value;
      
      // Dispatch event
      this.dispatchEvent(new CustomEvent('drum-type-change', {
        bubbles: true,
        detail: { 
          value,
          drum: this.currentDrum 
        }
      }));
    });
  }
  
  // Method to switch the active drum
  setActiveDrum(drumId) {
    if (this.drumsData[drumId]) {
      this.currentDrum = drumId;
      this.render();
      this.setupEventListeners();
      
      // Notify about drum change
      this.dispatchEvent(new CustomEvent('drum-change', {
        bubbles: true,
        detail: { 
          drum: this.currentDrum,
          name: this.drumsData[this.currentDrum].name
        }
      }));
      
      return true;
    }
    return false;
  }
}
// Register the custom element
customElements.define('drum-panel', DrumPanel);
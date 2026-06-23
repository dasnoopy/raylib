// Wait for translations to be loaded from the Blade template
document.addEventListener('DOMContentLoaded', function () {
    if (typeof window.jsTranslations === 'undefined') {
        console.error("JavaScript translations are not loaded. Make sure the translations script block is in your Blade file before this script.");
        return;
    }

    // Complete resistor color data - using translations
    const colorDatabase = {
        black:  { name: window.jsTranslations.colors.black,  digit: 0, multiplier: 1,          tolerance: null, tempCoeff: 250, color: '#000000' },
        brown:  { name: window.jsTranslations.colors.brown,  digit: 1, multiplier: 10,         tolerance: 1,    tempCoeff: 100, color: '#8B4513' },
        red:    { name: window.jsTranslations.colors.red,    digit: 2, multiplier: 100,        tolerance: 2,    tempCoeff: 50,  color: '#FF0000' },
        orange: { name: window.jsTranslations.colors.orange, digit: 3, multiplier: 1000,       tolerance: 0.05, tempCoeff: 15,  color: '#FFA500' },
        yellow: { name: window.jsTranslations.colors.yellow, digit: 4, multiplier: 10000,      tolerance: 0.02, tempCoeff: 25,  color: '#FFFF00' },
        green:  { name: window.jsTranslations.colors.green,  digit: 5, multiplier: 100000,     tolerance: 0.5,  tempCoeff: 20,  color: '#008000' },
        blue:   { name: window.jsTranslations.colors.blue,   digit: 6, multiplier: 1000000,    tolerance: 0.25, tempCoeff: 10,  color: '#0000FF' },
        violet: { name: window.jsTranslations.colors.violet, digit: 7, multiplier: 10000000,   tolerance: 0.1,  tempCoeff: 5,   color: '#8A2BE2' },
        grey:   { name: window.jsTranslations.colors.grey,   digit: 8, multiplier: 100000000,  tolerance: 0.01, tempCoeff: 1,   color: '#808080' },
        white:  { name: window.jsTranslations.colors.white,  digit: 9, multiplier: 1000000000, tolerance: null, tempCoeff: null,color: '#FFFFFF' },
        gold:   { name: window.jsTranslations.colors.gold,   digit: null, multiplier: 0.1,     tolerance: 5,    tempCoeff: null,color: '#FFD700' },
        silver: { name: window.jsTranslations.colors.silver, digit: null, multiplier: 0.01,    tolerance: 10,   tempCoeff: null,color: '#C0C0C0' },
        pink:   { name: window.jsTranslations.colors.pink,   digit: null, multiplier: 0.001,   tolerance: null, tempCoeff: null,color: '#FFC0CB' }
    };

    // State management
    let currentBandCount = 4;
    let selectedColors = {}; // Will be initialized later

    // Band configuration definitions - using translations
    const bandConfigs = {
        3: {
            bands: [
                { position: 1, type: 'digit',     label: window.jsTranslations.band_configs.digit1,     cannotBeBlack: true },
                { position: 2, type: 'digit',     label: window.jsTranslations.band_configs.digit2,     cannotBeBlack: false },
                { position: 3, type: 'multiplier',label: window.jsTranslations.band_configs.multiplier, cannotBeBlack: false }
            ],
            defaultTolerance: 20
        },
        4: {
            bands: [
                { position: 1, type: 'digit',     label: window.jsTranslations.band_configs.digit1,     cannotBeBlack: true },
                { position: 2, type: 'digit',     label: window.jsTranslations.band_configs.digit2,     cannotBeBlack: false },
                { position: 3, type: 'multiplier',label: window.jsTranslations.band_configs.multiplier, cannotBeBlack: false },
                { position: 4, type: 'tolerance', label: window.jsTranslations.band_configs.tolerance,  cannotBeBlack: false }
            ]
        },
        5: {
            bands: [
                { position: 1, type: 'digit',     label: window.jsTranslations.band_configs.digit1,     cannotBeBlack: true },
                { position: 2, type: 'digit',     label: window.jsTranslations.band_configs.digit2,     cannotBeBlack: false },
                { position: 3, type: 'digit',     label: window.jsTranslations.band_configs.digit3,     cannotBeBlack: false },
                { position: 4, type: 'multiplier',label: window.jsTranslations.band_configs.multiplier, cannotBeBlack: false },
                { position: 5, type: 'tolerance', label: window.jsTranslations.band_configs.tolerance,  cannotBeBlack: false }
            ]
        },
        6: {
            bands: [
                { position: 1, type: 'digit',     label: window.jsTranslations.band_configs.digit1,     cannotBeBlack: true },
                { position: 2, type: 'digit',     label: window.jsTranslations.band_configs.digit2,     cannotBeBlack: false },
                { position: 3, type: 'digit',     label: window.jsTranslations.band_configs.digit3,     cannotBeBlack: false },
                { position: 4, type: 'multiplier',label: window.jsTranslations.band_configs.multiplier, cannotBeBlack: false },
                { position: 5, type: 'tolerance', label: window.jsTranslations.band_configs.tolerance,  cannotBeBlack: false },
                { position: 6, type: 'tempCoeff', label: window.jsTranslations.band_configs.tcr,        cannotBeBlack: false }
            ]
        }
    };
    
    // Global function to be called from inline onchange attribute
    window.selectColor = function(position, colorKey) {
        selectedColors[position] = colorKey;
        updateResistorVisual();
        updateBandLabels();
        calculateResistance();
    }
    
    function selectBandCount(bands) {
        currentBandCount = bands;
        document.querySelectorAll('.band-option').forEach(btn => btn.classList.remove('active'));
        document.querySelector(`[data-bands="${bands}"]`).classList.add('active');
        generateCompleteTable();
        updateResistorVisual();
        updateBandLabels();
        calculateResistance();
    }

    function generateCompleteTable() {
        const config = bandConfigs[currentBandCount];
        const tableHeader = document.getElementById('table-header');
        const tableBody = document.getElementById('color-table-body');
        tableHeader.innerHTML = '';
        tableBody.innerHTML = '';
        const headerRow1 = document.createElement('tr');
        const headerRow2 = document.createElement('tr');
        headerRow1.innerHTML = `<th></th>`;
        headerRow2.innerHTML = `<th>${window.jsTranslations.table_headers.color}</th>`;

        config.bands.forEach((band, index) => {
            headerRow1.innerHTML += `<th>${window.jsTranslations.table_headers.band} ${band.position}</th>`;
            headerRow2.innerHTML += `<th>${getBandHeaderContent(band.type, index + 1)}</th>`;
        });
        tableHeader.appendChild(headerRow1);
        tableHeader.appendChild(headerRow2);

        Object.entries(colorDatabase).forEach(([colorKey, colorData]) => {
            const row = document.createElement('tr');
            const nameCell = document.createElement('td');
            nameCell.style.backgroundColor = colorData.color;
            nameCell.className = 'color-name-cell';
            const lightColors = ['orange', 'yellow', 'white', 'gold', 'silver', 'pink'];
            if (lightColors.includes(colorKey)) {
                nameCell.style.color = 'black';
                nameCell.style.textShadow = '1px 1px 2px rgba(255,255,255,0.8)';
            } else {
                nameCell.style.color = 'white';
                nameCell.style.textShadow = '1px 1px 2px rgba(0,0,0,0.8)';
            }
            nameCell.textContent = colorData.name;
            row.appendChild(nameCell);
            config.bands.forEach(band => {
                const cell = document.createElement('td');
                cell.innerHTML = generateCellContent(colorKey, colorData, band);
                row.appendChild(cell);
            });
            tableBody.appendChild(row);
        });
    }

    function getBandHeaderContent(type, position) {
        const headers = { digit: { 1: window.jsTranslations.table_headers.digit1, 2: window.jsTranslations.table_headers.digit2, 3: window.jsTranslations.table_headers.digit3 } };
        if (type === 'digit' && headers.digit[position]) return headers.digit[position];
        switch (type) {
            case 'multiplier': return window.jsTranslations.table_headers.multiplier;
            case 'tolerance': return window.jsTranslations.table_headers.tolerance;
            case 'tempCoeff': return window.jsTranslations.table_headers.tcr;
            default: return '';
        }
    }

    function generateCellContent(colorKey, colorData, band) {
        const { type, position, cannotBeBlack } = band;
        let isValid = false;
        let displayValue = '';
        switch (type) {
            case 'digit':
                if (colorData.digit !== null && !(cannotBeBlack && colorKey === 'black')) {
                    isValid = true;
                    displayValue = colorData.digit.toString();
                }
                break;
            case 'multiplier':
                if (colorData.multiplier !== null) {
                    isValid = true;
                    displayValue = `×10<sup>${Math.log10(colorData.multiplier)}</sup>`;
                }
                break;
            case 'tolerance':
                if (colorData.tolerance !== null) {
                    isValid = true;
                    displayValue = `±${colorData.tolerance}%`;
                }
                break;
            case 'tempCoeff':
                if (colorData.tempCoeff !== null) {
                    isValid = true;
                    displayValue = `±${colorData.tempCoeff}`;
                }
                break;
        }
        if (isValid) {
            return `<input type="radio" name="band-${position}-select" value="${colorKey}" id="radio-${position}-${colorKey}" class="color-input" onchange="selectColor(${position}, '${colorKey}')" ${selectedColors[position] === colorKey ? 'checked' : ''}><label for="radio-${position}-${colorKey}" class="color-choice bg-${colorKey}">${displayValue}</label>`;
        } else {
            return `<div class="color-choice no-value">${window.jsTranslations.display.no_value}</div>`;
        }
    }

    function updateResistorVisual() {
        for (let i = 1; i <= 6; i++) {
            const bandElement = document.getElementById(`resistor-band-${i}`);
            const topLabel = document.getElementById(`top-label-${i}`);
            const nameLabel = document.getElementById(`band-name-${i}`);
            const isVisible = i <= currentBandCount;
            if (bandElement) bandElement.style.display = isVisible ? 'block' : 'none';
            if (topLabel) topLabel.style.display = isVisible ? 'block' : 'none';
            if (nameLabel) nameLabel.style.display = isVisible ? 'block' : 'none';
            if (isVisible && selectedColors[i]) {
                const colorData = colorDatabase[selectedColors[i]];
                if (colorData && bandElement) bandElement.setAttribute('fill', colorData.color);
            }
        }
        const config = bandConfigs[currentBandCount];
        config.bands.forEach((band, index) => {
            const label = document.getElementById(`top-label-${band.position}`);
            if (label) {
                const labelText = { digit: (index + 1).toString(), multiplier: '×', tolerance: '±', tempCoeff: window.jsTranslations.display.ppm };
                label.textContent = labelText[band.type] || '';
            }
        });
    }

    function updateBandLabels() {
        for (let i = 1; i <= 6; i++) {
            const nameLabel = document.getElementById(`band-name-${i}`);
            if (nameLabel && selectedColors[i]) {
                const colorData = colorDatabase[selectedColors[i]];
                if (colorData) nameLabel.textContent = colorData.name;
            }
        }
    }

    function calculateResistance() {
        const config = bandConfigs[currentBandCount];
        let digits = [], multiplier = 1, tolerance = config.defaultTolerance || 20, tempCoeff = null;
        config.bands.forEach(band => {
            const colorData = colorDatabase[selectedColors[band.position]];
            if (!colorData) return;
            switch (band.type) {
                case 'digit': if (colorData.digit !== null) digits.push(colorData.digit); break;
                case 'multiplier': if (colorData.multiplier !== null) multiplier = colorData.multiplier; break;
                case 'tolerance': if (colorData.tolerance !== null) tolerance = colorData.tolerance; break;
                case 'tempCoeff': if (colorData.tempCoeff !== null) tempCoeff = colorData.tempCoeff; break;
            }
        });
        let resistance = 0;
        if (digits.length === 2) resistance = (digits[0] * 10 + digits[1]) * multiplier;
        else if (digits.length === 3) resistance = (digits[0] * 100 + digits[1] * 10 + digits[2]) * multiplier;
        updateResultsDisplay(resistance, tolerance, tempCoeff);
    }

    function updateResultsDisplay(resistance, tolerance, tempCoeff) {
        const formattedResistance = formatResistanceValue(resistance);
        const minValue = resistance * (1 - tolerance / 100);
        const maxValue = resistance * (1 + tolerance / 100);
        document.getElementById('resistance-value').textContent = `${formattedResistance} ±${tolerance}%`;
        document.getElementById('min-value').textContent = formatResistanceValue(minValue);
        document.getElementById('max-value').textContent = formatResistanceValue(maxValue);
        const tempCoeffField = document.getElementById('temp-coeff-field');
        if (tempCoeff !== null) {
            document.getElementById('temp-coeff').textContent = `±${tempCoeff} ${window.jsTranslations.display.ppm_k}`;
            tempCoeffField.style.display = 'block';
        } else {
            tempCoeffField.style.display = 'none';
        }
    }

    function formatResistanceValue(value) {
        if (value >= 1e9) return `${(value / 1e9).toFixed(value % 1e9 === 0 ? 0 : 2)} GΩ`;
        if (value >= 1e6) return `${(value / 1e6).toFixed(value % 1e6 === 0 ? 0 : 2)} MΩ`;
        if (value >= 1e3) return `${(value / 1e3).toFixed(value % 1e3 === 0 ? 0 : 2)} kΩ`;
        return `${value.toFixed(value % 1 === 0 ? 0 : 2)} Ω`;
    }

    function toggleAdvanced() {
        const advancedResults = document.getElementById('advanced-results');
        const toggleButton = document.getElementById('advanced-toggle');
        const isShown = advancedResults.classList.toggle('show');
        toggleButton.textContent = isShown ? window.jsTranslations.ui.hide_advanced : window.jsTranslations.ui.show_advanced;
    }

    function showToast(message) {
        let toast = document.getElementById('toast');
        if (!toast) { // Create toast if it doesn't exist
            toast = document.createElement('div');
            toast.id = 'toast';
            document.body.appendChild(toast);
        }
        toast.textContent = message;
        toast.classList.add('show');
        setTimeout(() => toast.classList.remove('show'), 3000);
    }

    function copyLink() {
        navigator.clipboard.writeText(window.location.href)
            .then(() => showToast(window.jsTranslations.ui.link_copied))
            .catch(() => showToast('Failed to copy link.')); // Fallback message
    }

    function initializeCalculator() {
        selectedColors = { 1: 'brown', 2: 'red', 3: 'red', 4: 'gold', 5: 'brown', 6: 'black' };
        selectBandCount(4); // Set default state to 4 bands and trigger initial calculation
    }

    // Assign share functions to window object to be accessible from HTML onclick
    window.shareOnFacebook = () => window.open(`https://www.facebook.com/sharer/sharer.php?u=${encodeURIComponent(window.location.href)}"e=${encodeURIComponent(window.jsTranslations.share.facebook_quote)}`, '_blank', 'width=555,height=333');
    window.shareOnTwitter = () => window.open(`https://twitter.com/intent/tweet?text=${encodeURIComponent(window.jsTranslations.share.twitter_text)}&url=${encodeURIComponent(window.location.href)}`, '_blank', 'width=555,height=333');
    window.shareOnReddit = () => window.open(`https://reddit.com/submit?url=${encodeURIComponent(window.location.href)}&title=${encodeURIComponent(window.jsTranslations.share.reddit_title)}`, '_blank', 'width=555,height=333');
    window.shareOnPinterest = () => {
        const media = encodeURIComponent(window.location.origin + '/resistor-preview.png');
        window.open(`https://pinterest.com/pin/create/button/?url=${encodeURIComponent(window.location.href)}&description=${encodeURIComponent(window.jsTranslations.share.pinterest_description)}&media=${media}`, '_blank', 'width=555,height=333');
    };
    window.shareByEmail = () => {
        const body = window.jsTranslations.share.email_body.replace(':url', window.location.href);
        window.location.href = `mailto:?subject=${encodeURIComponent(window.jsTranslations.share.email_subject)}&body=${encodeURIComponent(body)}`;
    };
    window.copyLink = copyLink;

    // Event listeners
    initializeCalculator();
    document.querySelectorAll('.band-option').forEach(btn => btn.addEventListener('click', () => selectBandCount(parseInt(btn.dataset.bands))));
    document.getElementById('advanced-toggle').addEventListener('click', toggleAdvanced);
});
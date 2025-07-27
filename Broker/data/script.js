let statusInterval;
const REFRESH_INTERVAL = 5000;
let gaugeTemp, gaugeHum, gaugeGas, gaugeSoil;

function initGauges() {
    const opts = { angle: -0.25, lineWidth: 0.2, radiusScale: 0.9, pointer: { length: 0.5, strokeWidth: 0.045, color: '#333' }, limitMax: false, limitMin: false, strokeColor: '#E0E0E0', generateGradient: true, highDpiSupport: true, };
    const tempTarget = document.getElementById('gauge-temp');
    if (tempTarget) { gaugeTemp = new Gauge(tempTarget).setOptions(opts); gaugeTemp.maxValue = 50; gaugeTemp.setMinValue(0); gaugeTemp.animationSpeed = 32; gaugeTemp.set(0); }
    const humTarget = document.getElementById('gauge-hum');
    if (humTarget) { gaugeHum = new Gauge(humTarget).setOptions(opts); gaugeHum.maxValue = 100; gaugeHum.setMinValue(0); gaugeHum.animationSpeed = 32; gaugeHum.set(0); }
    const gasTarget = document.getElementById('gauge-gas');
    if (gasTarget) { gaugeGas = new Gauge(gasTarget).setOptions(opts); gaugeGas.maxValue = 4095; gaugeGas.setMinValue(0); gaugeGas.animationSpeed = 32; gaugeGas.set(0); }
    const soilTarget = document.getElementById('gauge-soil');
    if (soilTarget) { gaugeSoil = new Gauge(soilTarget).setOptions(opts); gaugeSoil.maxValue = 100; gaugeSoil.setMinValue(0); gaugeSoil.animationSpeed = 32; gaugeSoil.set(0); }
}

function openTab(evt, tabName) {
    var i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
    clearInterval(statusInterval);
    if (tabName === 'Dashboard') {
        getStatus();
        statusInterval = setInterval(getStatus, REFRESH_INTERVAL);
    } else if (tabName === 'Settings') {
        getStatus();
    }
}

function handleModeChange() {
    const modeSelect = document.getElementById('mode-select');
    const manualControls = document.getElementById('manual-controls-container');
    if (modeSelect.value == 5) {
        manualControls.style.display = 'block';
    } else {
        manualControls.style.display = 'none';
    }
}

function updateUI(data) {
    document.getElementById('nav-ip').innerText = `IP: ${data.ip}`;
    document.getElementById('val-mode').innerText = data.mode;
    document.getElementById('val-temp').innerText = `${data.temp} \u00B0C`;
    document.getElementById('val-hum').innerText = `${data.hum} %`;
    document.getElementById('val-gas').innerText = data.gas;
    document.getElementById('val-soil').innerText = `${data.soil} %`;
    if (gaugeTemp) gaugeTemp.set(data.temp);
    if (gaugeHum) gaugeHum.set(data.hum);
    if (gaugeGas) gaugeGas.set(data.gas);
    if (gaugeSoil) gaugeSoil.set(data.soil);
    const relays = ['r1', 'r2', 'r3', 'r4', 'r5'];
    data.relays.forEach((status, i) => {
        const el = document.getElementById(`val-${relays[i]}`);
        const checkbox = document.getElementById(`relay-${i}`);
        if (status) {
            if (el) { el.innerText = "ON"; el.className = "value on"; }
            if (checkbox) checkbox.checked = true;
        } else {
            if (el) { el.innerText = "OFF"; el.className = "value off"; }
            if (checkbox) checkbox.checked = false;
        }
    });
    document.getElementById('mode-select').value = data.mode_val;
    document.getElementById('tempMin-input').value = data.settings.temp_min;
    document.getElementById('tempMax-input').value = data.settings.temp_max;
    document.getElementById('humMin-input').value = data.settings.hum_min;
    document.getElementById('humMax-input').value = data.settings.hum_max;
    document.getElementById('gasLimit-input').value = data.settings.gas_limit;
    document.getElementById('relayDelay-input').value = data.settings.relay_delay;
    handleModeChange();
}

async function getStatus() {
    try {
        const response = await fetch('/status');
        const data = await response.json();
        updateUI(data);
    } catch (error) {
        console.error('Error fetching status:', error);
    }
}

function saveSettings() {
    const mode = document.getElementById('mode-select').value;
    const tempMin = document.getElementById('tempMin-input').value;
    const tempMax = document.getElementById('tempMax-input').value;
    const humMin = document.getElementById('humMin-input').value;
    const humMax = document.getElementById('humMax-input').value;
    const gasLimit = document.getElementById('gasLimit-input').value;
    const relayDelay = document.getElementById('relayDelay-input').value;
    let url = `/set?mode=${mode}&temp_min=${tempMin}&temp_max=${tempMax}&hum_min=${humMin}&hum_max=${humMax}&gas_limit=${gasLimit}&relay_delay=${relayDelay}`;
    if (mode == 5) {
        for (let i = 0; i < 5; i++) {
            const checkbox = document.getElementById(`relay-${i}`);
            if (checkbox) {
                url += `&r${i}=${checkbox.checked}`;
            }
        }
    }
    fetch(url)
        .then(response => response.text())
        .then(data => { console.log(data); alert("Settings saved!"); getStatus(); })
        .catch(error => console.error('Error saving settings:', error));
}

function rebootDevice() {
    if (confirm("Bạn có chắc muốn khởi động lại thiết bị?")) {
        fetch('/reboot').then(response => response.text()).then(data => alert(data)).catch(error => console.error('Error rebooting:', error));
    }
}

function resetWifi() {
    if (confirm("Cảnh báo: Thao tác này sẽ xóa toàn bộ cài đặt Wi-Fi đã lưu và khởi động lại thiết bị. Bạn có chắc chắn?")) {
        fetch('/resetwifi').then(response => response.text()).then(data => { alert(data + " Vui lòng kết nối vào mạng 'Broker-Setup-Portal' để cài đặt lại."); }).catch(error => console.error('Error resetting WiFi:', error));
    }
}

window.onload = function () {
    document.getElementById('mode-select').addEventListener('change', handleModeChange);
    initGauges();
    document.querySelector('.tablinks').click();
}
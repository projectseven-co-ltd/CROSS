import { invoke } from '@tauri-apps/api/tauri';

// ============================================================================
// AP MODE SETUP
// ============================================================================

let apDeviceInfo = null;

async function handleAPConnect() {
  const status = document.getElementById('apStatus');
  status.textContent = 'Connecting to device...';
  status.style.display = 'block';
  
  try {
    const response = await fetch('http://192.168.4.1/api/status', {
      method: 'GET',
      headers: { 'Content-Type': 'application/json' },
      signal: AbortSignal.timeout(5000)
    });
    
    if (response.ok) {
      apDeviceInfo = await response.json();
      status.textContent = `Connected to ${apDeviceInfo.name}`;
      status.style.color = 'green';
      document.getElementById('apSetupForm').style.display = 'block';
      await loadAPWiFiNetworks();
    } else {
      status.textContent = 'Device error';
      status.style.color = 'red';
    }
  } catch (error) {
    status.textContent = 'Device not found. Make sure you\'re connected to cross.X WiFi.';
    status.style.color = 'red';
    console.error(error);
  }
}

async function loadAPWiFiNetworks() {
  const select = document.getElementById('apWiFiSSID');
  select.innerHTML = '<option value="">Scanning...</option>';
  
  try {
    const response = await fetch('http://192.168.4.1/api/scan-wifi', {
      method: 'GET',
      headers: { 'Content-Type': 'application/json' }
    });
    
    if (response.ok) {
      const networks = await response.json();
      select.innerHTML = '<option value="">Select network...</option>';
      networks.forEach(network => {
        const option = document.createElement('option');
        option.value = network.ssid;
        option.textContent = `${network.ssid} (${network.encryption})`;
        select.appendChild(option);
      });
    }
  } catch (error) {
    console.error('WiFi scan error:', error);
  }
}

async function handleAPWiFiConfig(e) {
  e.preventDefault();
  const status = document.getElementById('apStatus');
  const ssid = document.getElementById('apWiFiSSID').value.trim();
  const password = document.getElementById('apWiFiPassword').value;
  
  if (!ssid) {
    status.textContent = 'Error: Please select a network';
    status.style.color = 'red';
    return;
  }
  
  status.textContent = 'Configuring WiFi...';
  
  try {
    const response = await fetch('http://192.168.4.1/api/configure-wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password: password || '' })
    });
    
    if (response.ok) {
      status.textContent = 'WiFi configured! Device restarting...';
      status.style.color = 'green';
      document.getElementById('apSetupForm').style.display = 'none';
      apDeviceInfo = null;
      
      setTimeout(() => {
        location.reload();
      }, 3000);
    } else {
      status.textContent = 'Configuration failed';
      status.style.color = 'red';
    }
  } catch (error) {
    status.textContent = `Error: ${error.message}`;
    status.style.color = 'red';
    console.error(error);
  }
}

// ============================================================================
// NETWORK DISCOVERY (mDNS)
// ============================================================================

async function handleDiscovery() {
  const status = document.getElementById('discoveryStatus');
  status.textContent = 'Discovering devices on network...';
  status.style.display = 'block';
  
  try {
    const crosses = await invoke('discover_crosses');
    if (crosses.length === 0) {
      status.textContent = 'No devices found';
      return;
    }
    
    const list = document.getElementById('discoveredList');
    list.innerHTML = '';
    
    crosses.forEach(cross => {
      const item = document.createElement('div');
      item.className = 'device-item';
      item.innerHTML = `
        <strong>${cross.name}</strong><br>
        ${cross.ip_address}<br>
        <button onclick="handleDirectConnect('${cross.ip_address}')">Connect</button>
      `;
      list.appendChild(item);
    });
    
    status.textContent = `Found ${crosses.length} device(s)`;
    status.style.color = 'green';
  } catch (error) {
    status.textContent = `Discovery error: ${error}`;
    status.style.color = 'red';
  }
}

async function handleDirectConnect(ip) {
  const status = document.getElementById('discoveryStatus');
  status.textContent = `Connecting to ${ip}...`;
  
  try {
    const result = await invoke('test_connection', { ip });
    status.textContent = `Connected! Available endpoints: ${result.join(', ')}`;
    status.style.color = 'green';
  } catch (error) {
    status.textContent = `Connection failed: ${error}`;
    status.style.color = 'red';
  }
}

// ============================================================================
// PRAYERS
// ============================================================================

async function handlePrayerSubmit(e) {
  e.preventDefault();
  const text = document.getElementById('prayerText').value.trim();
  const status = document.getElementById('prayerStatus');
  
  if (!text) {
    status.textContent = 'Prayer text required';
    status.style.color = 'red';
    return;
  }
  
  status.textContent = 'Submitting prayer...';
  
  try {
    const result = await invoke('send_prayer', { text });
    status.textContent = `Prayer submitted! ID: ${result.id}`;
    status.style.color = 'green';
    document.getElementById('prayerText').value = '';
  } catch (error) {
    status.textContent = `Error: ${error}`;
    status.style.color = 'red';
  }
}

// ============================================================================
// PAGE INIT
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('apConnectBtn').addEventListener('click', handleAPConnect);
  document.getElementById('apConfigForm').addEventListener('submit', handleAPWiFiConfig);
  document.getElementById('discoverBtn').addEventListener('click', handleDiscovery);
  document.getElementById('prayerForm').addEventListener('submit', handlePrayerSubmit);
});

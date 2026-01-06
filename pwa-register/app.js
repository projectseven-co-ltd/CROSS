// CROSS Registration PWA App
// Simple vanilla JS app (no build tools needed)

// Configuration
const CONFIG = {
  // Update these to match your NocoDB instance
  NOCODB_URL: 'https://noco.app.p7n.net',
  API_TOKEN: 'fDhJb1s9aK8yQsj99iSt6DOe9o518yGAwAdwezn1',
  
  // Table IDs from NocoDB
  USERS_TABLE: 'mtsl8eo5bfxhg8h',      // Adjust if needed
  CROSSES_TABLE: 'mdk4garchpkpvsd',
  PRAYERS_TABLE: 'mt7iucs253e26u0',
};

// Local state
let currentUser = null;

// Initialize app
document.addEventListener('DOMContentLoaded', () => {
  restoreSession();
});

// ============================================================================
// AUTHENTICATION
// ============================================================================

function switchTab(tab) {
  const loginTab = document.getElementById('loginTab');
  const signupTab = document.getElementById('signupTab');
  const tabs = document.querySelectorAll('.tab-btn');

  if (tab === 'login') {
    loginTab.style.display = 'block';
    signupTab.style.display = 'none';
    tabs[0].classList.add('active');
    tabs[1].classList.remove('active');
  } else {
    loginTab.style.display = 'none';
    signupTab.style.display = 'block';
    tabs[0].classList.remove('active');
    tabs[1].classList.add('active');
  }
}

async function handleLogin() {
  const email = document.getElementById('loginEmail').value.trim();
  const password = document.getElementById('loginPassword').value;

  if (!email || !password) {
    showAlert('authScreen', 'Please enter email and password', 'error');
    return;
  }

  try {
    // In production, use proper authentication
    // For now, we'll create a simple user session
    const users = await fetchFromNocoDB(`${CONFIG.USERS_TABLE}/records`, {
      where: `(email,eq,${email})`
    });

    if (!users || users.length === 0) {
      showAlert('authScreen', 'User not found', 'error');
      return;
    }

    const user = users[0];
    
    // Simple password check (in production, use bcrypt on backend)
    if (password !== 'password123' && user.password !== password) {
      showAlert('authScreen', 'Invalid password', 'error');
      return;
    }

    currentUser = {
      id: user.Id,
      name: user.name,
      email: user.email
    };

    localStorage.setItem('currentUser', JSON.stringify(currentUser));
    showAlert('authScreen', 'Logged in successfully!', 'success');
    
    setTimeout(() => {
      goToScreen('homeScreen');
    }, 500);
  } catch (error) {
    console.error('Login error:', error);
    showAlert('authScreen', 'Login failed: ' + error.message, 'error');
  }
}

async function handleSignup() {
  const name = document.getElementById('signupName').value.trim();
  const email = document.getElementById('signupEmail').value.trim();
  const password = document.getElementById('signupPassword').value;
  const passwordConfirm = document.getElementById('signupPasswordConfirm').value;

  if (!name || !email || !password) {
    showAlert('authScreen', 'Please fill in all fields', 'error');
    return;
  }

  if (password !== passwordConfirm) {
    showAlert('authScreen', 'Passwords do not match', 'error');
    return;
  }

  try {
    const newUser = {
      name,
      email,
      password, // Note: In production, hash this on backend
    };

    const result = await postToNocoDB(`${CONFIG.USERS_TABLE}/records`, newUser);
    
    currentUser = {
      id: result.Id,
      name: result.name,
      email: result.email
    };

    localStorage.setItem('currentUser', JSON.stringify(currentUser));
    showAlert('authScreen', 'Account created successfully!', 'success');
    
    setTimeout(() => {
      goToScreen('homeScreen');
    }, 500);
  } catch (error) {
    console.error('Signup error:', error);
    showAlert('authScreen', 'Sign up failed: ' + error.message, 'error');
  }
}

function handleLogout() {
  currentUser = null;
  localStorage.removeItem('currentUser');
  document.getElementById('loginEmail').value = '';
  document.getElementById('loginPassword').value = '';
  goToScreen('authScreen');
}

function restoreSession() {
  const saved = localStorage.getItem('currentUser');
  if (saved) {
    try {
      currentUser = JSON.parse(saved);
      goToScreen('homeScreen');
    } catch (e) {
      goToScreen('authScreen');
    }
  }
}

// ============================================================================
// SCREENS
// ============================================================================

function goToScreen(screenId) {
  // Hide all screens
  document.querySelectorAll('.screen').forEach(s => {
    s.classList.remove('active');
  });

  // Show target screen
  document.getElementById(screenId).classList.add('active');

  // Load screen-specific data
  if (screenId === 'homeScreen') {
    updateUserDisplay();
  } else if (screenId === 'myCrossesScreen') {
    loadUserCrosses();
  } else if (screenId === 'prayerScreen') {
    loadCrossesForPrayer();
  }
}

function updateUserDisplay() {
  document.getElementById('userDisplayName').textContent = currentUser.name || 'User';
  document.getElementById('userDisplayEmail').textContent = currentUser.email;
}

// ============================================================================
// CROSS REGISTRATION
// ============================================================================

async function handleRegisterCross() {
  const deviceName = document.getElementById('deviceName').value.trim();
  const geoArea = document.getElementById('geoArea').value;
  const locationNotes = document.getElementById('locationNotes').value.trim();

  if (!deviceName) {
    showAlert('registerCrossScreen', 'Please enter device name', 'error');
    return;
  }

  if (!geoArea) {
    showAlert('registerCrossScreen', 'Please select a geographic area', 'error');
    return;
  }

  try {
    // Find cross by device_name
    const crosses = await fetchFromNocoDB(`${CONFIG.CROSSES_TABLE}/records`, {
      where: `(device_name,eq,${deviceName})`
    });

    if (!crosses || crosses.length === 0) {
      showAlert('registerCrossScreen', 'Cross device not found. Check the device name.', 'error');
      return;
    }

    const cross = crosses[0];
    
    // Update cross with user info
    const updateData = {
      user_id: currentUser.id,
      geo_area: geoArea,
      location_notes: locationNotes,
      status: 'active'
    };

    await patchToNocoDB(`${CONFIG.CROSSES_TABLE}/records`, cross.Id, updateData);

    showAlert('registerCrossScreen', 'Cross registered successfully!', 'success');
    
    // Clear form
    document.getElementById('deviceName').value = '';
    document.getElementById('geoArea').value = '';
    document.getElementById('locationNotes').value = '';
    
    setTimeout(() => {
      goToScreen('homeScreen');
    }, 1000);
  } catch (error) {
    console.error('Registration error:', error);
    showAlert('registerCrossScreen', 'Registration failed: ' + error.message, 'error');
  }
}

// ============================================================================
// MY CROSSES
// ============================================================================

async function loadUserCrosses() {
  const container = document.getElementById('crossesList');
  
  try {
    const crosses = await fetchFromNocoDB(`${CONFIG.CROSSES_TABLE}/records`, {
      where: `(user_id,eq,${currentUser.id})`
    });

    if (!crosses || crosses.length === 0) {
      container.innerHTML = `
        <p class="text-sm text-center" style="color: #666; padding: 20px;">
          No crosses registered yet.<br>
          <span class="link" onclick="goToScreen('registerCrossScreen')">Register a cross</span>
        </p>
      `;
      return;
    }

    container.innerHTML = crosses.map(cross => `
      <div class="prayer-item">
        <div class="prayer-text" style="font-weight: 500; margin-bottom: 8px;">
          ${cross.device_name || 'Unnamed Cross'}
        </div>
        <div class="user-info-item" style="border: none; padding: 4px 0;">
          <span class="user-info-label">Area:</span>
          <span class="user-info-value">${cross.geo_area || 'N/A'}</span>
        </div>
        <div class="user-info-item" style="border: none; padding: 4px 0;">
          <span class="user-info-label">Prayers:</span>
          <span class="user-info-value">${cross.intercession_count || 0}</span>
        </div>
        ${cross.location_notes ? `
          <div class="user-info-item" style="border: none; padding: 4px 0;">
            <span class="user-info-label">Location:</span>
            <span class="user-info-value">${cross.location_notes}</span>
          </div>
        ` : ''}
        <div class="text-sm" style="margin-top: 8px; color: #666;">
          Device Key: ${cross.device_key}
        </div>
        <button class="btn-secondary" style="margin-top: 12px; font-size: 13px;" onclick="generateCrossQR('${cross.device_key}')">
          View QR Code
        </button>
      </div>
    `).join('');
  } catch (error) {
    console.error('Load crosses error:', error);
    container.innerHTML = `<p class="alert alert-error">Error loading crosses: ${error.message}</p>`;
  }
}

function generateCrossQR(deviceKey) {
  // Create modal or dialog with QR code
  const qrData = JSON.stringify({
    device_key: deviceKey,
    register_url: window.location.origin + '/pwa-register'
  });

  const modal = document.createElement('div');
  modal.style.cssText = `
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(0,0,0,0.8);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 1000;
  `;

  modal.innerHTML = `
    <div style="background: #1a1a1a; padding: 20px; border-radius: 12px; max-width: 400px; width: 90%;">
      <h3 style="margin-bottom: 16px; text-align: center;">Cross QR Code</h3>
      <div id="qrModal" style="text-align: center; background: #fff; padding: 16px; border-radius: 8px; margin-bottom: 16px;"></div>
      <p class="text-sm text-center" style="color: #999; margin-bottom: 16px;">Scan with your phone to register this cross</p>
      <button class="btn-primary" onclick="this.parentElement.parentElement.remove()">Close</button>
    </div>
  `;

  document.body.appendChild(modal);

  // Generate QR code
  setTimeout(() => {
    new QRCode(document.getElementById('qrModal'), {
      text: qrData,
      width: 300,
      height: 300,
      correctLevel: QRCode.CorrectLevel.H
    });
  }, 100);
}

// ============================================================================
// PRAYERS
// ============================================================================

async function loadCrossesForPrayer() {
  const select = document.getElementById('prayerCrossSelect');
  
  try {
    const crosses = await fetchFromNocoDB(`${CONFIG.CROSSES_TABLE}/records`, {
      where: `(user_id,eq,${currentUser.id})`
    });

    if (!crosses || crosses.length === 0) {
      select.innerHTML = '<option value="">No crosses registered</option>';
      return;
    }

    select.innerHTML = '<option value="">Select a cross...</option>' + 
      crosses.map(cross => 
        `<option value="${cross.Id}">${cross.device_name || 'Unnamed'}</option>`
      ).join('');
  } catch (error) {
    console.error('Load crosses error:', error);
  }
}

async function handleSubmitPrayer() {
  const crossId = document.getElementById('prayerCrossSelect').value;
  const prayerText = document.getElementById('prayerText').value.trim();

  if (!crossId) {
    showAlert('prayerScreen', 'Please select a cross', 'error');
    return;
  }

  if (!prayerText) {
    showAlert('prayerScreen', 'Please enter a prayer request', 'error');
    return;
  }

  try {
    const prayer = {
      cross_id: crossId,
      user_id: currentUser.id,
      prayer_text: prayerText,
      status: 'submitted'
    };

    await postToNocoDB(`${CONFIG.PRAYERS_TABLE}/records`, prayer);

    showAlert('prayerScreen', 'Prayer submitted successfully!', 'success');
    
    document.getElementById('prayerText').value = '';
    document.getElementById('prayerCrossSelect').value = '';
    
    setTimeout(() => {
      goToScreen('homeScreen');
    }, 1000);
  } catch (error) {
    console.error('Prayer submission error:', error);
    showAlert('prayerScreen', 'Failed to submit prayer: ' + error.message, 'error');
  }
}

// ============================================================================
// NOCO API HELPERS
// ============================================================================

async function fetchFromNocoDB(endpoint, params = {}) {
  let url = `${CONFIG.NOCODB_URL}/api/v2/tables/${endpoint}`;
  
  if (params.where) {
    url += `?where=${params.where}`;
  }
  if (params.limit) {
    url += `${params.where ? '&' : '?'}limit=${params.limit}`;
  }

  const response = await fetch(url, {
    headers: {
      'xc-token': CONFIG.API_TOKEN,
      'Content-Type': 'application/json'
    }
  });

  if (!response.ok) {
    throw new Error(`NocoDB fetch failed: ${response.status}`);
  }

  const data = await response.json();
  return data.list || data.data || [];
}

async function postToNocoDB(endpoint, payload) {
  const url = `${CONFIG.NOCODB_URL}/api/v2/tables/${endpoint}`;
  
  const response = await fetch(url, {
    method: 'POST',
    headers: {
      'xc-token': CONFIG.API_TOKEN,
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(payload)
  });

  if (!response.ok) {
    throw new Error(`NocoDB POST failed: ${response.status}`);
  }

  return response.json();
}

async function patchToNocoDB(endpoint, recordId, payload) {
  const url = `${CONFIG.NOCODB_URL}/api/v2/tables/${endpoint}?where=(Id,eq,${recordId})`;
  
  const response = await fetch(url, {
    method: 'PATCH',
    headers: {
      'xc-token': CONFIG.API_TOKEN,
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      Id: recordId,
      ...payload
    })
  });

  if (!response.ok) {
    throw new Error(`NocoDB PATCH failed: ${response.status}`);
  }

  return response.json();
}

// ============================================================================
// UI HELPERS
// ============================================================================

function showAlert(screenId, message, type = 'info') {
  const screen = document.getElementById(screenId);
  const alertDiv = document.createElement('div');
  alertDiv.className = `alert alert-${type}`;
  alertDiv.textContent = message;
  
  const card = screen.querySelector('.card');
  if (card) {
    card.insertBefore(alertDiv, card.firstChild);
  }

  // Auto-remove after 5 seconds
  setTimeout(() => {
    alertDiv.remove();
  }, 5000);
}

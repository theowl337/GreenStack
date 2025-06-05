// Copyright 2025 Theodor Hein
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

function saveThemeToCookie(theme) {
  document.cookie = `theme=${theme}; expires=Fri, 31 Dec 9999 23:59:59 GMT; path=/`;
}

function getThemeFromCookie() {
  const match = document.cookie.match(/theme=([^;]+)/);
  return match ? match[1] : 'dark';
}

function toggleTheme() {
  const currentTheme =
    document.documentElement.getAttribute('data-theme') || 'dark';
  const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
  setTheme(newTheme);
}

function setTheme(theme) {
  document.documentElement.setAttribute('data-theme', theme);

  const themeIcons = document.querySelectorAll('.theme-icon');
  themeIcons.forEach((icon) => {
    icon.textContent = theme === 'dark' ? '☀️' : '🌙';
  });

  saveThemeToCookie(theme);
}

function initializeTheme() {
  const savedTheme = getThemeFromCookie();
  setTheme(savedTheme);
}

function togglePump() {
  fetch('/pump_on')
    .then((response) => {
      if (response.ok) {
        console.log('Pump activated');
        const button = document.querySelector('.button-holder button');
        const originalHTML = button.innerHTML;
        button.innerHTML = '<span class="button-icon">⏳</span>Watering...';
        button.disabled = true;

        setTimeout(() => {
          button.innerHTML = originalHTML;
          button.disabled = false;
        }, 5000);
      }
    })
    .catch((error) => {
      console.error('Error:', error);
      const button = document.querySelector('.button-holder button');
      button.innerHTML = 'Error';
      setTimeout(() => {
        button.innerHTML = '<span class="button-icon">💧</span>Water Plant';
      }, 2000);
    });
}

function updateCardValue(elementId, value, unit) {
  const element = document.getElementById(elementId);
  if (element) {
    element.style.transition = 'all 0.3s ease';
    element.style.transform = 'scale(0.9)';
    element.style.opacity = '0.7';

    setTimeout(() => {
      element.innerText = value + (unit || '');
      element.style.transform = 'scale(1)';
      element.style.opacity = '1';
    }, 150);
  }
}

function fetchTemperature() {
  fetch('/temperature')
    .then((response) => response.json())
    .then((data) => {
      updateCardValue('temp', data.temp, ' °C');
    })
    .catch((error) => {
      console.error('Error fetching temperature:', error);
      updateCardValue('temp', 'Error', '');
    });
}

function fetchHumidity() {
  fetch('/humidity')
    .then((response) => response.json())
    .then((data) => {
      updateCardValue('humidity', data.humidity, ' %');
    })
    .catch((error) => {
      console.error('Error fetching humidity:', error);
      updateCardValue('humidity', 'Error', '');
    });
}

function fetchSoilMoisture() {
  fetch('/soilmoisture')
    .then((response) => response.json())
    .then((data) => {
      updateCardValue('soilmoisture', data.soilmoisture, '');
    })
    .catch((error) => {
      console.error('Error fetching soil moisture:', error);
      updateCardValue('soilmoisture', 'Error', '');
    });
}

document.addEventListener('DOMContentLoaded', function () {
  initializeTheme();
});

setInterval(fetchTemperature, 2000);
setInterval(fetchHumidity, 2000);
setInterval(fetchSoilMoisture, 2000);

window.addEventListener('load', function () {
  fetchTemperature();
  fetchHumidity();
  fetchSoilMoisture();
  checkWiFiStatus();
});

function openWiFiConfig() {
  document.getElementById('wifi-modal').style.display = 'block';
  document.body.style.overflow = 'hidden';
  checkWiFiStatus();
}

function closeWiFiConfig() {
  document.getElementById('wifi-modal').style.display = 'none';
  document.body.style.overflow = '';
}

function checkWiFiStatus() {
  fetch('/wifi/status')
    .then((response) => response.json())
    .then((data) => {
      const statusIcon = document.getElementById('wifi-status-icon');
      const statusText = document.getElementById('wifi-status-text');

      if (data.connected) {
        statusText.textContent = `Connected to: ${data.ssid} (${data.rssi} dBm)`;
        statusText.style.color = 'var(--primary-green)';
      } else if (data.ap_mode) {
        statusText.textContent = `AP Mode: ${data.ap_ssid}`;
        statusText.style.color = 'var(--secondary-blue)';
      } else {
        statusText.textContent = 'Not connected';
        statusText.style.color = 'var(--text-secondary)';
      }
    })
    .catch((error) => {
      console.error('Error checking WiFi status:', error);
      document.getElementById('wifi-status-text').textContent =
        'Status check failed';
    });
}

function togglePassword() {
  const passwordInput = document.getElementById('password');
  const toggleButton = document.querySelector('.toggle-password');

  if (passwordInput.type === 'password') {
    passwordInput.type = 'text';
    toggleButton.textContent = '🙈';
  } else {
    passwordInput.type = 'password';
    toggleButton.textContent = '👁️';
  }
}

function connectWiFi(event) {
  event.preventDefault();

  const ssid = document.getElementById('ssid').value;
  const password = document.getElementById('password').value;
  const connectButton = document.querySelector('.connect-button');

  if (!ssid) {
    alert('Please enter a network name');
    return;
  }

  connectButton.innerHTML = '<span class="connect-icon">⏳</span>Connecting...';
  connectButton.disabled = true;

  const data = {
    ssid: ssid,
    password: password,
  };

  fetch('/wifi/connect', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        connectButton.innerHTML = 'LETS GOO!';
        setTimeout(() => {
          closeWiFiConfig();
          window.location.reload();
        }, 2000);
      } else {
        connectButton.innerHTML = 'Failed!!';
        alert('Connection failed: ' + (data.message || ' error'));
        setTimeout(() => {
          connectButton.innerHTML = 'Connect';
          connectButton.disabled = false;
        }, 2000);
      }
    })
    .catch((error) => {
      console.error('Error connecting to WiFi:', error);
      connectButton.innerHTML = 'Error';
      alert('Connection error');
      setTimeout(() => {
        connectButton.innerHTML = 'Connect';
        connectButton.disabled = false;
      }, 2000);
    });
}

function toggleAPMode() {
  const apButton = document.querySelector('.ap-button');

  apButton.innerHTML = '<span class="ap-icon">⏳</span>Switching...';
  apButton.disabled = true;

  fetch('/wifi/toggle_ap', {
    method: 'POST',
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        apButton.innerHTML = 'Switched!';
        setTimeout(() => {
          closeWiFiConfig();
          window.location.reload();
        }, 2000);
      } else {
        apButton.innerHTML = '<span class="ap-icon">❌</span>Failed';
        alert('AP mode toggle failed: ' + (data.message || 'Unknown error'));
      }
    })
    .catch((error) => {
      console.error('Error toggling AP mode:', error);
      apButton.innerHTML = '<span class="ap-icon">❌</span>Error';
      alert('AP mode toggle error');
    })
    .finally(() => {
      setTimeout(() => {
        apButton.innerHTML = 'Toggle AP Mode';
        apButton.disabled = false;
      }, 2000);
    });
}

window.onclick = function (event) {
  const modal = document.getElementById('wifi-modal');
  if (event.target === modal) {
    closeWiFiConfig();
  }
};

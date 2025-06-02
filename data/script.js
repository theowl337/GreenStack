function togglePump() {
  fetch('/pump_on')
    .then((response) => {
      if (response.ok) {
        console.log('Pump activated');
        // Optional: Visuelle Bestätigung
        const button = document.querySelector('.button-holder button');
        const originalText = button.textContent;
        button.textContent = 'Watering...';
        button.disabled = true;

        setTimeout(() => {
          button.textContent = originalText;
          button.disabled = false;
        }, 5000);
      }
    })
    .catch((error) => console.error('Error:', error));
}

function fetchTemperature() {
  fetch('/temperature')
    .then((response) => response.json())
    .then((data) => {
      document.getElementById('temp').innerText = data.temp + ' °C';
    })
    .catch((error) => {
      console.error('Error fetching temperature:', error);
      document.getElementById('temp').innerText = 'Error';
    });
}

function fetchHumidity() {
  fetch('/humidity')
    .then((response) => response.json())
    .then((data) => {
      document.getElementById('humidity').innerText = data.humidity + ' %';
    })
    .catch((error) => {
      console.error('Error fetching humidity:', error);
      document.getElementById('humidity').innerText = 'Error';
    });
}

function fetchSoilMoisture() {
  fetch('/soilmoisture')
    .then((response) => response.json())
    .then((data) => {
      document.getElementById('soilmoisture').innerText = data.soilmoisture;
    })
    .catch((error) => {
      console.error('Error fetching soil moisture:', error);
      document.getElementById('soilmoisture').innerText = 'Error';
    });
}

// Update Intervalle
setInterval(fetchTemperature, 2000);
setInterval(fetchHumidity, 2000);
setInterval(fetchSoilMoisture, 2000);

// Initial load
window.addEventListener('load', function () {
  fetchTemperature();
  fetchHumidity();
  fetchSoilMoisture();
});

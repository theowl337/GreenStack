function togglePump() {
  fetch("/pump_on");
}

function fetchTemperature() {
  fetch("/temperature")
    .then(response => response.json())
    .then(data => {
      document.getElementById("temp").innerText = data.temp + " °C";
    });
}

function fetchHumidity() {
  fetch("/humidity")
    .then(response => response.json())
    .then(data => {
      document.getElementById("humidity").innerText = data.humidity + " %";
    });
}

function fetchSoilMoisture() {
  fetch("/soilmoisture")
    .then(response => response.json())
    .then(data => {
      document.getElementById("soilmoisture").innerText = data.soilmoisture;
    });
}

setInterval(fetchTemperature, 2000);
setInterval(fetchHumidity, 2000);
setInterval(fetchSoilMoisture, 2000);
window.onload = fetchTemperature;

# GreenStack
Greenstack is a smart solution for watering potted plants.

# Details
Greenstack is based on the design of a hanging basket for potted plants.
However, this design features multiple hanging baskets stacked on top of each other.
The water that is not used by the above plant is funneled to the one below.
This allows for less water to be used up over time.

Greenstack adds to this functionality by letting you control it over your network connection.
It let's you control how much to automatically water the plants.
The integrated sensors even let you keep track of your plants and when they are running dry,
as well as temperature and humidity readings of the room.

# Materials needed
1. ESP32 module, preferably ESP-WROOM-32
2. Water pump and tube
3. Relay module
4. DHT22 temperature and humidity sensor
5. Hygrometer (soil moisture sensor, capacitive)
6. Jumper cables
7. 3D-printed casing


# Technical Features

- Web server running on ESP32 with RESTful API endpoints
- SPIFFS file system for storing web interface files
- FreeRTOS tasks for non-blocking pump control
- NTP time synchronization --> logging
- Automatic WiFi fallback to AP mode
- JSON API for sensor data

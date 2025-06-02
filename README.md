# GreenStack
Greanstack is a smart solution for watering potted plants.

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
4. Water pump and tube
5. IRLZ44N MOSFET
6. 12V to 5V Buck converter
7. DHT22 temperature and humidity sensor
8. Hygrometer (soil moisture sensor, capacitive)
9. 3D-printed casing


# Technical Features

1. Web server running on ESP32 with RESTful API endpoints
2. SPIFFS file system for storing web interface files
3. FreeRTOS tasks for non-blocking pump control
4. NTP time synchronization --> logging
5. Automatic WiFi fallback to AP mode
6. JSON API for sensor data

# Weather Station Project

This project involves an ESP32-based weather station that reads data from a MISOL weather sensor and stores the results on a microSD card. The weather station also acts as an access point (AP) to serve a web interface, allowing users to view weather data and configure settings.

## Hardware

- **Microcontroller**: ESP32  
- **Sensors**: MISOL Weather Station  
- **Storage**: MicroSD card  
- **Miscellaneous**: Relay, built-in LED  

## Features

1. **ESP32 Access Point**:  
   The ESP32 is set up as an access point (AP) with the SSID `weather_station` and password `sulungresearch`. The AP has a static IP of `192.168.8.1`. The web interface allows users to:
   - View weather data
   - Configure Wi-Fi and device settings
   - Download or delete stored data from the microSD card

2. **Watchdog Timer**:  
   A watchdog timer resets the ESP32 if it becomes unresponsive for more than 60 seconds.

3. **Data Storage and Retrieval**:  
   Weather data is stored on a microSD card in CSV format. The system also allows users to download or delete specific files via the web interface.

4. **JSON Data Construction**:  
   Weather data is formatted into JSON for external usage or display purposes.

5. **Wi-Fi Settings and Static IP**:  
   Users can configure Wi-Fi settings and static IP addresses, which are saved to a file (`settings.json`) on the microSD card.

## Web Interface

The ESP32 serves a web interface where users can:
- View current weather station data
- Configure settings like SSID, password, and static IP
- List files on the microSD card, download, or delete them
- View the serial monitor in real-time

## File Handling

- Data is saved to `/data.txt` in the format:
YYYY-MM-DD HH:MM:ss, windspeedkmh, winddir, rainratein, temp_in, temp_out, humidity_in, humidity_out, UV, windgustkmh, baromrelin, baromabsin, solarradiation
- Configuration is stored in `/settings.json`, including:
  

{
  "ssid": "your_ssid",
  "password": "your_password",
  "id": 1,
  "staticIP": "10.9.116.174",
  "gateway": "10.9.116.1",
  "subnet": "255.255.255.0",
  "dnsServer": "192.168.1.22"
}

## Example Data
Here’s an example of weather data stored on the SD card:
2024-01-01 08:00:00, 5.5, 180, 0.0, 22.3, 27.5, 55, 60, 5.0, 6.0, 1013.2, 1012.1, 700

This data represents:

- Date and time: `2024-01-01 08:00:00`
- Windspeed (km/h): `5.5`
- Wind direction (degrees): `180`
- Rain rate (inches per hour): `0.0`
- Indoor temperature (°C): `22.3`
- Outdoor temperature (°C): `27.5`
- Indoor humidity (%): `55`
- Outdoor humidity (%): `60`
- UV index: `5.0`
- Wind gust (km/h): `6.0`
- Barometric pressure relative (inches of Hg): `1013.2`
- Barometric pressure absolute (inches of Hg): `1012.1`
- Solar radiation (W/m²): `700`

## Example JSON Data

When requested, the weather data is also provided in JSON format. Here is an example JSON structure:


{
  "datetime": "2024-01-01 08:00:00",
  "windspeed_kmh": 5.5,
  "wind_direction": 180,
  "rain_rate_in": 0.0,
  "temperature_in_c": 22.3,
  "temperature_out_c": 27.5,
  "humidity_in": 55,
  "humidity_out": 60,
  "uv_index": 5.0,
  "wind_gust_kmh": 6.0,
  "barometric_pressure_rel_in": 1013.2,
  "barometric_pressure_abs_in": 1012.1,
  "solar_radiation_wm2": 700
}
This JSON structure can be used by external applications or for displaying data on a remote server. It allows for easy integration with IoT platforms, APIs, or web services that can process and visualize weather data in real-time.

## Code Overview

### Main Components

1. **Wi-Fi Connection**:  
   The ESP32 attempts to connect to a pre-configured Wi-Fi network on startup. If no network is found or credentials are invalid, it switches to access point (AP) mode, allowing users to connect directly to the device to configure settings via a web interface.

2. **SD Card Handling**:  
   Weather data is stored on the SD card in CSV format. Users can download or delete these files through the web interface. The SD card is also used to store configuration settings such as Wi-Fi credentials.

3. **JSON Construction**:  
   The device can generate JSON data from the weather sensor readings. This JSON format can be used to provide real-time data through an API endpoint or sent to an external server for further processing.

4. **Web Server**:  
   The web server provides several routes for interaction:
   - `/` displays the current weather data and allows configuration of the Wi-Fi and system settings.
   - `/save` handles saving Wi-Fi credentials or static IP configurations.
   - `/post` allows external applications to post data (e.g., new sensor readings).
   - `/serial` streams real-time weather data via the serial connection.
   - `/download` provides the ability to download weather data files stored on the SD card.
   - `/delete` allows users to delete data files from the SD card.

## Watchdog Timer

To ensure reliability, the ESP32 uses a watchdog timer that is set to 60 seconds. This mechanism helps to automatically restart the system if it becomes unresponsive for any reason, ensuring continuous operation without manual intervention.

## Setup and Configuration

In the `setup()` function, the following tasks are performed:

- **Wi-Fi Initialization**:  
  The system attempts to connect to a Wi-Fi network using the credentials stored in the `settings.json` file. If no connection can be established, the ESP32 switches to access point (AP) mode, allowing users to connect and configure the device through a local web interface.

- **SD Card Initialization**:  
  The SD card is mounted, and existing weather data is accessed for logging purposes.

- **Web Server Initialization**:  
  The web server is started to handle HTTP requests for configuration, file management, and data retrieval.

### Wi-Fi Configuration

Wi-Fi credentials and static IP settings are stored on the SD card in a file named `settings.json`. Users can modify these settings through the web interface. The file structure looks like this:


{
  "ssid": "your_ssid",
  "password": "your_password",
  "id": 1,
  "staticIP": "192.168.8.1",
  "gateway": "192.168.8.1",
  "subnet": "255.255.255.0",
  "dnsServer": "8.8.8.8"
}



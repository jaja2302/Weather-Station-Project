#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
ESP8266WebServer server(80);
int csPin = 0;
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Ticker.h>
WebServer server(80);
int csPin = 5;
const int relayPin = 13;
#endif

#include <SD.h>
#include <RTClib.h>
#include <Timer.h>

// ESP32 Access Point credentials
const char *ap_ssid = "weather_station";    // SSID for the AP
const char *ap_password = "sulungresearch"; // Password for the AP

// Static IP settings for the ESP32 AP
IPAddress ap_local_ip(192, 168, 8, 1);
IPAddress ap_gateway(192, 168, 8, 1);
IPAddress ap_subnet(255, 255, 255, 0);

#define WATCHDOG_TIMEOUT 60
volatile int watchdogMin = 0;

#define SERIAL_BUFFER_SIZE 20
String serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferIndex = 0;

void addToSerialBuffer(const String &message);

String zeroDate(int zero);
void appendFile(fs::FS &fs, const char *path, String message);
String constructJsonData(const String values[], int size);
void connectWiFi();
void sendData();
void deleteTopLine();
void loadSettings();
void saveSettings();
void handleRoot();
void handleSaveSettings();
void handleDownload();
void handleDelete();

int watchdogTimer = 11;
int delayMill = 3000;

File myFile;

int id, testLoop = 0;

Timer sendTimer;

IPAddress staticIP, gateway, subnet, dnsServer;

Ticker watchdogTicker;

String payload;

bool checkSend = false;

void addToSerialBuffer(const String &message)
{
  Serial.println(message); // Still print to actual serial for debugging
  serialBuffer[serialBufferIndex] = message;
  serialBufferIndex = (serialBufferIndex + 1) % SERIAL_BUFFER_SIZE;
}

struct Settings
{
  String ssid;
  String password;
  int id;
  IPAddress staticIP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dnsServer;
};

Settings settings;

void resetWatchdog()
{
  watchdogMin++;
  if (watchdogMin >= watchdogTimer)
  {
    esp_cpu_reset(0);
    ESP.restart();
  }
}

void loadSettings()
{
  if (SD.exists("/settings.json"))
  {
    File file = SD.open("/settings.json", FILE_READ);
    if (file)
    {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, file);
      if (error)
      {
        addToSerialBuffer("Failed to read settings file");
      }
      else
      {
        settings.ssid = doc["ssid"].as<String>();
        settings.password = doc["password"].as<String>();
        settings.id = doc["id"].as<int>();
        settings.staticIP.fromString(doc["staticIP"].as<String>());
        settings.gateway.fromString(doc["gateway"].as<String>());
        settings.subnet.fromString(doc["subnet"].as<String>());
        settings.dnsServer.fromString(doc["dnsServer"].as<String>());
      }
      file.close();
    }
  }
  else
  {
    // Default settings
    settings.ssid = "SRS";
    settings.password = "SRS@2023";
    settings.id = 1;
    settings.staticIP.fromString("10.9.116.174");
    settings.gateway.fromString("10.9.116.1");
    settings.subnet.fromString("255.255.255.0");
    settings.dnsServer.fromString("192.168.1.22");
    saveSettings();
  }
}

void saveSettings()
{
  File file = SD.open("/settings.json", FILE_WRITE);
  if (file)
  {
    DynamicJsonDocument doc(1024);
    doc["ssid"] = settings.ssid;
    doc["password"] = settings.password;
    doc["id"] = settings.id;
    doc["staticIP"] = settings.staticIP.toString();
    doc["gateway"] = settings.gateway.toString();
    doc["subnet"] = settings.subnet.toString();
    doc["dnsServer"] = settings.dnsServer.toString();
    if (serializeJson(doc, file) == 0)
    {
      addToSerialBuffer("Failed to write settings file");
    }
    file.close();
  }
  else
  {
    addToSerialBuffer("Failed to open settings file for writing");
  }
}

void handleRoot()
{
  String html = "<html><body>";
  html += "<h1>Weather Station Settings</h1>";
  html += "<form action='/save' method='POST'>";
  html += "SSID: <input type='text' name='ssid' value='" + settings.ssid + "'><br>";
  html += "Password: <input type='password' name='password' value='" + settings.password + "'><br>";
  html += "ID: <input type='number' name='id' value='" + String(settings.id) + "'><br>";
  html += "Static IP: <input type='text' name='staticIP' value='" + settings.staticIP.toString() + "'><br>";
  html += "Gateway: <input type='text' name='gateway' value='" + settings.gateway.toString() + "'><br>";
  html += "Subnet: <input type='text' name='subnet' value='" + settings.subnet.toString() + "'><br>";
  html += "DNS Server: <input type='text' name='dnsServer' value='" + settings.dnsServer.toString() + "'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";

  // Add SD card file listing with download and delete options
  html += "<h2>SD Card Files</h2>";
  html += "<table border='1'><tr><th>File Name</th><th>Actions</th></tr>";
  File root = SD.open("/");
  while (File file = root.openNextFile())
  {
    String fileName = String(file.name());
    html += "<tr><td>" + fileName + "</td><td>";
    html += "<a href='/download?file=" + fileName + "'>Download</a> | ";
    html += "<a href='/delete?file=" + fileName + "' onclick='return confirm(\"Are you sure you want to delete this file?\")'>Delete</a>";
    html += "</td></tr>";
    file.close();
  }
  html += "</table>";

  html += "<h2>Serial Monitor</h2>";
  html += "<pre id='serial'></pre>";
  html += "<script>setInterval(() => fetch('/serial').then(r => r.text()).then(t => document.getElementById('serial').textContent = t), 1000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleDelete()
{
  String fileName = server.arg("file");
  if (SD.exists("/" + fileName))
  {
    if (SD.remove("/" + fileName))
    {
      server.sendHeader("Location", "/");
      server.send(303);
    }
    else
    {
      server.send(500, "text/plain", "Failed to delete file");
    }
  }
  else
  {
    server.send(404, "text/plain", "File not found");
  }
}

void handleDownload()
{
  String fileName = server.arg("file");
  if (SD.exists("/" + fileName))
  {
    File file = SD.open("/" + fileName, FILE_READ);
    if (file)
    {
      server.sendHeader("Content-Type", "text/plain");
      server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
      server.streamFile(file, "text/plain");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain", "File not found");
}

void handleSaveSettings()
{
  settings.ssid = server.arg("ssid");
  settings.password = server.arg("password");
  settings.id = server.arg("id").toInt();
  settings.staticIP.fromString(server.arg("staticIP"));
  settings.gateway.fromString(server.arg("gateway"));
  settings.subnet.fromString(server.arg("subnet"));
  settings.dnsServer.fromString(server.arg("dnsServer"));
  saveSettings();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSerial()
{
  String output;
  for (int i = 0; i < SERIAL_BUFFER_SIZE; i++)
  {
    int index = (serialBufferIndex + i) % SERIAL_BUFFER_SIZE;
    if (serialBuffer[index].length() > 0)
    {
      output += serialBuffer[index] + "\n";
    }
  }
  server.send(200, "text/plain", output);
}

void handlePost()
{
  if (server.method() == HTTP_POST)
  {
    String dateutc, baromabsin, windgustmph, baromrelin, date_jkt, solarradiation, windgustkmh, windspeedkmh, windspeedmph, winddir, rainratein, tempf, tempinf, humidityin, uv, humidity, dateStr = "";
    float temp_in, temp_out = 0;
    DateTime datetimeNow, datetimePlus;

    dateutc = server.arg("dateutc");

    addToSerialBuffer("DATEUTC: " + String(dateutc));

    uint16_t year = 0;
    uint8_t month, day, hour, minute, second = 0;
    // Parse the input date-time string
    year = dateutc.substring(0, 4).toInt();
    month = dateutc.substring(5, 7).toInt();
    day = dateutc.substring(8, 10).toInt();
    hour = dateutc.substring(11, 13).toInt();
    minute = dateutc.substring(14, 16).toInt();
    second = dateutc.substring(17, 19).toInt();

    String substr = "substr: " + String(year) + "," + String(month) + "," + String(day) + "," + String(hour) + "," + String(minute) + "," + String(second);

    // Add 7 hours
    hour += 7;

    // Adjust the date and time values if necessary
    if (hour >= 24)
    {
      hour -= 24;
      day += 1;
    }
    if (month == 2 && day > 28)
    { // Adjust for February having 28 days
      day = 1;
      month += 1;
    }
    else if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
    { // Adjust for months with 30 days
      day = 1;
      month += 1;
    }
    else if (day > 31)
    { // Adjust for months with 31 days
      day = 1;
      month += 1;
    }
    if (month > 12)
    { // Adjust for changing years
      month = 1;
      year += 1;
    }

    String month_str = zeroDate(month);
    String day_str = zeroDate(day);
    String hour_str = zeroDate(hour);
    String minute_str = zeroDate(minute);
    String second_str = zeroDate(second);

    // Construct the adjusted date string
    String new_date_string = String(year) + "-" + String(month_str) + "-" + String(day_str) + " " + String(hour_str) + ":" + String(minute_str) + ":" + String(second_str);
    // Print the adjusted date string
    // addToSerialBuffer(new_date_string);

    tempinf = server.arg("tempinf");
    tempf = server.arg("tempf");
    windspeedmph = server.arg("windspeedmph");
    windgustmph = server.arg("windgustmph");

    windgustmph = windgustmph.toFloat() * 1.60934;
    windspeedkmh = windspeedmph.toFloat() * 1.60934;
    winddir = server.arg("winddir");
    rainratein = server.arg("rainratein");
    temp_in = (5.0 / 9.0) * (tempinf.toFloat() - 32.0);
    humidityin = server.arg("humidityin");
    uv = server.arg("uv");
    temp_out = (5.0 / 9.0) * (tempf.toFloat() - 32.0);
    humidity = server.arg("humidity");
    solarradiation = server.arg("solarradiation");
    baromrelin = server.arg("baromrelin");
    baromabsin = server.arg("baromabsin");

    String data = new_date_string + "," + windspeedkmh + "," + winddir + "," + rainratein + "," + temp_in + "," + temp_out + "," + humidityin + "," + humidity + "," + uv + "," + windgustmph + "," + baromrelin + "," + baromabsin + "," + solarradiation;

    if (data != "")
    {
      checkSend = true;
    }

    appendFile(SD, "/data.txt", String(data));

    server.send(200, "text/plain", "Data saved to SD card.");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void setup()
{
  Serial.begin(115200);

  watchdogTicker.attach(WATCHDOG_TIMEOUT, resetWatchdog);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  delay(4000);
  digitalWrite(relayPin, HIGH);

  if (!SD.begin(csPin))
  {
    addToSerialBuffer("Card failed, or not present");
  }
  addToSerialBuffer("Card initialized successfully");

  loadSettings();

  connectWiFi();

  // Set up ESP32 as an Access Point with the specified IP and credentials
  WiFi.softAPConfig(ap_local_ip, ap_gateway, ap_subnet); // Configure AP with static IP
  WiFi.softAP(ap_ssid, ap_password);                     // Start the Access Point

  Serial.println("ESP32 Access Point started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP()); // Should be 192.168.8.1

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSaveSettings);
  server.on("/post", handlePost);
  server.on("/serial", handleSerial);
  server.on("/download", handleDownload); // Add this line
  server.on("/delete", handleDelete);     // Add this line

  server.begin();
  addToSerialBuffer("Server started");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  sendTimer.every(delayMill, sendData);
}

#include <ArduinoJson.h>

// ... (previous code remains the same)

String constructJsonData(const String values[], int size)
{
  DynamicJsonDocument doc(1024);
  doc["idws"] = settings.id;

  const char *keys[] = {"date", "windspeedkmh", "winddir", "rain_rate", "temp_in", "temp_out",
                        "hum_in", "hum_out", "uv", "wind_gust", "air_press_rel",
                        "air_press_abs", "solar_radiation"};

  for (int i = 0; i < size; i++)
  {
    String value = values[i];
    value.trim(); // Remove leading/trailing whitespace and control characters

    if (value.length() > 0 && value != "NULL" && value != "null")
    {
      if (i == 0)
      {
        doc[keys[i]] = value; // Store date as a string
      }
      else if (i == 1 && (value == "0" || value == "0.00"))
      {
        doc[keys[i]] = 0.00; // Specifically handle 0 or 0.00 for windspeedkmh
      }
      else if (value.toFloat() != 0 || value == "0")
      {
        doc[keys[i]] = value.toFloat();
      }
      else
      {
        doc[keys[i]] = value;
      }
    }
    else
    {
      doc[keys[i]] = JsonVariant(); // This creates a null value in JSON
    }
  }

  String output;
  serializeJson(doc, output);
  return output;
}

void sendData()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    addToSerialBuffer("WiFi disconnected");
    connectWiFi();
  }

  String rawText;
  myFile = SD.open("/data.txt");
  if (myFile)
  {
    rawText = myFile.readStringUntil('\n');
    myFile.close();

    // Check if the data is empty or contains errors
    if (rawText.length() == 0 || rawText.indexOf("error") != -1)
    {
      addToSerialBuffer("Empty or error data found. Deleting line.");
      deleteTopLine();
      return; // Exit the function early
    }

    if (rawText != "")
    {
      String values[13];
      int index = 0;
      while (rawText.length() > 0 && index < 13)
      {
        int separatorIndex = rawText.indexOf(",");
        if (separatorIndex == -1)
        {
          values[index] = rawText;
          values[index].trim(); // Remove leading/trailing whitespace and control characters
          index++;
          break;
        }
        values[index] = rawText.substring(0, separatorIndex);
        values[index].trim(); // Remove leading/trailing whitespace and control characters
        index++;
        rawText = rawText.substring(separatorIndex + 1);
      }

      // Fill remaining values with empty strings if necessary
      while (index < 13)
      {
        values[index++] = "NULL";
      }

      // Validate date format
      if (values[0].length() != 19 || values[0][4] != '-' || values[0][7] != '-' || values[0][10] != ' ' || values[0][13] != ':' || values[0][16] != ':')
      {
        addToSerialBuffer("Invalid date format. Expected YYYY-MM-DD HH:MM:SS");
        return; // Exit the function if date format is invalid
      }

      WiFiClient client;
      HTTPClient http;

      String data = constructJsonData(values, 13);

      addToSerialBuffer("Attempting to send data: " + data);
      http.begin(client, "http://srs-ssms.com/iot/post-data-aws.php");
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.POST(data);

      if (httpCode == 200)
      {
        String response = http.getString();
        addToSerialBuffer("HTTP response: " + response);
        deleteTopLine();
        addToSerialBuffer("Data sent successfully. Line deleted.");
      }
      else if (httpCode > 0)
      {
        String response = http.getString();
        addToSerialBuffer("HTTP error response: " + response);
      }
      else
      {
        addToSerialBuffer("HTTP error: " + String(httpCode));
      }
      http.end();
    }
    else
    {
      addToSerialBuffer("No data to send.");
    }
  }
  else
  {
    addToSerialBuffer("Error opening file!");
  }

  Serial.print("loop ke ");
  testLoop++;
  addToSerialBuffer(String(testLoop));
}

void connectWiFi()
{
  WiFi.config(settings.staticIP, settings.gateway, settings.subnet, settings.dnsServer);
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
  Serial.print("Connecting to ");
  addToSerialBuffer(settings.ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to ssid ");
  addToSerialBuffer(settings.ssid);
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  addToSerialBuffer("/");
  Serial.print("Gateway: ");
  addToSerialBuffer(String(WiFi.gatewayIP()));
  Serial.print("Subnet mask: ");
  addToSerialBuffer(String(WiFi.subnetMask()));
}

void deleteTopLine()
{
  File originalFile = SD.open("/data.txt", FILE_READ);
  File newFile = SD.open("/new_file.txt", FILE_WRITE);

  if (!originalFile || !newFile)
  {
    addToSerialBuffer("Error opening files for deletion process");
    return;
  }

  originalFile.readStringUntil('\n'); // Skip the first line

  while (originalFile.available())
  {
    String line = originalFile.readStringUntil('\n');
    if (line.length() > 0)
    {
      newFile.println(line);
    }
  }

  originalFile.close();
  newFile.close();

  if (SD.remove("/data.txt"))
  {
    if (SD.rename("/new_file.txt", "/data.txt"))
    {
      addToSerialBuffer("Top line deleted successfully");
    }
    else
    {
      addToSerialBuffer("Error renaming file");
    }
  }
  else
  {
    addToSerialBuffer("Error removing original file");
    SD.remove("/new_file.txt"); // Clean up the new file if we couldn't remove the original
  }
}

void loop()
{
  server.handleClient();
  sendTimer.update();
  watchdogMin = 0;
}

void appendFile(fs::FS &fs, const char *path, String message)
{
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    addToSerialBuffer("- failed to open file for appending");
    return;
  }
  if (file.println(message))
  {
    addToSerialBuffer("- message appended");
  }
  else
  {
    addToSerialBuffer("- append failed");
  }
  file.close();
}

String zeroDate(int zero)
{
  String month_str = String(zero);
  if (month_str.length() == 1)
  {
    month_str = "0" + month_str;
  }
  return month_str;
}
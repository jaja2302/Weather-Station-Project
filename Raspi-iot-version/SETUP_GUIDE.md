# Weather Station Setup Guide - Raspberry Pi

## Quick Start

### 1. Install on Raspberry Pi

```bash
# Clone or copy the Raspi-iot-version folder to your Raspberry Pi
cd /home/pi/weather-station

# Make scripts executable
chmod +x install.sh run.sh test_weather_data.py

# Run installation
./install.sh
```

### 2. Start the Service

```bash
# Start the weather station service
sudo systemctl start weather_station.service

# Check if it's running
sudo systemctl status weather_station.service

# View logs
journalctl -u weather_station.service -f
```

### 3. Access Web Interface

Open your browser and go to: `http://YOUR_PI_IP:5000`

## Manual Setup (Alternative)

### 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y python3 python3-pip python3-venv sqlite3
```

### 2. Setup Virtual Environment

```bash
cd /home/pi/weather-station
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### 3. Run Application

```bash
# Quick run
./run.sh

# Or run directly
python3 weather_station.py
```

## ESP32 Integration

### 1. Update ESP32 Settings

Copy the `esp32_settings.json` file to your ESP32's microSD card as `settings.json` and update:

```json
{
  "ssid": "YOUR_WIFI_SSID",
  "password": "YOUR_WIFI_PASSWORD", 
  "id": 1,
  "postUrl": "http://YOUR_PI_IP:5000/post"
}
```

### 2. Test Connection

```bash
# Test with sample data
python3 test_weather_data.py

# Test continuous data
python3 test_weather_data.py continuous 30
```

## Configuration

### Web Interface Settings

Access `http://YOUR_PI_IP:5000` and configure:

- **SSID**: Your WiFi network name
- **Password**: Your WiFi password
- **Station ID**: Unique identifier for this station
- **Post URL**: Where ESP32 should send data (usually `http://YOUR_PI_IP:5000/post`)

### File Locations

- **Database**: `data/weather.db`
- **CSV Data**: `data/weather_data.csv`
- **Settings**: `data/settings.json`
- **Logs**: `logs/weather_station.log`

## API Usage

### Send Data from ESP32

```bash
# Form data (compatible with original ESP32 code)
curl -X POST http://YOUR_PI_IP:5000/post \
  -d "dateutc=2024-01-01 12:00:00" \
  -d "tempf=75.2" \
  -d "windspeedmph=8.5" \
  -d "humidity=65"
```

### Send JSON Data

```bash
curl -X POST http://YOUR_PI_IP:5000/api/weather \
  -H "Content-Type: application/json" \
  -d '{
    "datetime": "2024-01-01 12:00:00",
    "windspeed_kmh": 13.7,
    "temp_out_c": 24.0,
    "humidity_out": 65
  }'
```

### Get Latest Data

```bash
curl http://YOUR_PI_IP:5000/api/weather/latest
```

## Troubleshooting

### Service Won't Start

```bash
# Check service status
sudo systemctl status weather_station.service

# Check logs
journalctl -u weather_station.service -f

# Check if port is in use
sudo netstat -tlnp | grep :5000
```

### Database Issues

```bash
# Check database
sqlite3 data/weather.db ".tables"

# Backup database
cp data/weather.db data/weather_backup.db
```

### Permission Issues

```bash
# Fix ownership
sudo chown -R pi:pi /home/pi/weather-station

# Fix permissions
chmod +x weather_station.py
chmod 755 data logs
```

### Network Issues

```bash
# Check if service is listening
sudo netstat -tlnp | grep :5000

# Test local connection
curl http://localhost:5000

# Check firewall
sudo ufw status
```

## Development Mode

```bash
# Activate virtual environment
source venv/bin/activate

# Run with debug
export FLASK_DEBUG=1
python3 weather_station.py
```

## Data Export

### Export to CSV

```bash
# Export all data
sqlite3 -header -csv data/weather.db "SELECT * FROM weather_data;" > export.csv

# Export recent data (last 7 days)
sqlite3 -header -csv data/weather.db "SELECT * FROM weather_data WHERE created_at > datetime('now', '-7 days');" > recent_export.csv
```

### Backup Database

```bash
# Create backup
cp data/weather.db "backup_$(date +%Y%m%d_%H%M%S).db"

# Restore from backup
cp backup_20240101_120000.db data/weather.db
```

## Monitoring

### System Resources

```bash
# Check CPU and memory usage
htop

# Check disk usage
df -h

# Check service status
systemctl status weather_station.service
```

### Log Monitoring

```bash
# Follow logs in real-time
tail -f logs/weather_station.log

# Search for errors
grep -i error logs/weather_station.log

# Check recent activity
tail -n 100 logs/weather_station.log
```

## Security Notes

- Change default passwords
- Use HTTPS in production
- Configure firewall rules
- Regular backups
- Monitor system resources
- Keep system updated

## Support

For issues and questions:
1. Check the logs first
2. Verify network connectivity
3. Check service status
4. Review configuration files
5. Test with sample data

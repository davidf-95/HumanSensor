 #include "MyLD2410.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>
#include <DNSServer.h>

#define EEPROM_SIZE 128
#define MIN_DELAY_NEXT_WHATSAPP_MSG 2*60*1000   //1MIN
//#define MIN_TRIGGER_DURATION_NEXT_WHATSAPP_MSG 2*60*1000   //1MIN
//#define MAX_PER_HOURS_WHATSAPP_MSG 40   //  40  (1000PER DAY)

#define RX_PIN 22
#define TX_PIN 23
#define SERIAL_BAUD_RATE 115200
#define LD2410_BAUD_RATE 256000
#define LED_PIN 2

const char *ssid = "Wifi_David";
const char *password = "thecakeisalie";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
DNSServer dnsServer;

HardwareSerial sensorSerial(1);
MyLD2410 sensor(sensorSerial);


struct Movement {
    String timestamp;
    int distance;
    int stationaryDistance;
    bool DetectedMovementTarget;
};

#define MAX_LOG 1000
Movement movementLog[MAX_LOG];
int logIndex = 0;
const int DEFAULT_MOVING_THRESHOLDS[9] = {50, 50, 40, 30, 20, 15, 15, 15, 15};
const int DEFAULT_STATIONARY_THRESHOLDS[9] = {0, 0, 40, 40, 30, 30, 20, 20, 20};

unsigned long lastMovementTime = 0;
int minDetectionTime = 500;
bool alarmEnabled = false;
int alarmVolume = 0;
bool movementDetected = false;
bool whatsappNotificationsEnabled = true;
bool recordAllDiagramData = false;

int sensorResolution = 5;
int maxMovingGate = 7;
int maxStationaryGate = 7;

bool wlan_ap_mode = false;
MyLD2410::ValuesArray movingParameters, stationaryParameters;

void addMovement(int distance, int stationaryDistance, bool isMovementTarget ) {
    if (logIndex >= MAX_LOG) {
        for (int i = 1; i < MAX_LOG; i++) {
            movementLog[i - 1] = movementLog[i];
        }
        logIndex--;
    }
    movementLog[logIndex].timestamp = timeClient.getFormattedTime();
    movementLog[logIndex].distance = distance;
    movementLog[logIndex].stationaryDistance = stationaryDistance;
    movementLog[logIndex].DetectedMovementTarget = isMovementTarget;
    
    logIndex++;
}

void handleLogData() {
    String json = "{ \"log\": [";
    for (int i = 0; i < logIndex; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"time\":\"" + movementLog[i].timestamp + "\",";
        json += "\"distance\":" + String(movementLog[i].distance) + ",";
        json += "\"stationaryDistance\":" + String(movementLog[i].stationaryDistance) + "," ;
        json += "\"movemendDetected\":" + String(movementLog[i].DetectedMovementTarget);
        json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void handleRoot() {
    movingParameters = sensor.getMovingThresholds();
    stationaryParameters = sensor.getStationaryThresholds();
    int rssi = WiFi.RSSI();

    String html = "<html><head>";
    html += "<title>Bewegungssensor</title>";
    html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
    html += "<style>";
    html += "body { font-family: sans-serif; }";
    html += "table { width: 100%; border-collapse: collapse; }";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html += "th { background-color: #f2f2f2; }";
    html += "</style>";
    html += "<script>";
    html += "var socket = new WebSocket('ws://' + window.location.hostname + ':81/');";
    html += "var alarmSound = new Audio('https://cdn.pixabay.com/download/audio/2021/08/04/audio_e55afcf5a0.mp3?filename=beep-beep-6151.mp3');";
    html += "var alarmPlaying = false;";
    html += "alarmSound.volume = " + String(alarmVolume / 100.0) + ";"; // Lautstärke setzen

    html += "socket.onmessage = function(event) {";
    html += "    if (event.data === 'motion_detected' && Notification.permission === 'granted') {";
    html += "        sendPush();";
    html += "    }";
    html += "};";

    html += "function requestNotificationPermission() {";
    html += "    if (Notification.permission !== 'granted') {";
    html += "        Notification.requestPermission().then(permission => {";
    html += "            if (permission === 'granted') { alert('Benachrichtigungen aktiviert!'); }";
    html += "            else { alert('Benachrichtigungen wurden abgelehnt.'); }";
    html += "        });";
    html += "    } else { alert('Benachrichtigungen sind bereits aktiviert.'); }";
    html += "}";

    html += "function sendPush() {";
    html += "    if (Notification.permission === 'granted') {";
    html += "        new Notification('🚨 Bewegung erkannt!', {";
    html += "            body: 'Der Sensor hat eine Bewegung registriert.'";
    html += "        });";
    html += "    }";
    html += "}";

    html += "document.addEventListener('DOMContentLoaded', function() {";
    html += "    var btn = document.createElement('button');";
    html += "    btn.innerHTML = 'Benachrichtigungen aktivieren';";
    html += "    btn.onclick = requestNotificationPermission;";
    html += "    document.body.appendChild(btn);";
    html += "});";

    html += "function updateChart(){";
    html += "    fetch('/logdata').then(res=>res.json()).then(data=>{";
    html += "        let labels=[], moveData=[], stillData=[] , movemendDetected=[]; ";
    html += "        let chartSizeOld = movementChart.data.datasets[0].data.length;";
    html += "        data.log.forEach(entry => {";
    html += "            labels.push(entry.time); moveData.push(entry.distance); stillData.push(entry.stationaryDistance);  movemendDetected.push(entry.movemendDetected); ";
    html += "        });";
    html += "        movementChart.data.labels = labels;";
    html += "        movementChart.data.datasets[0].data = moveData;";
    html += "        movementChart.data.datasets[1].data = stillData;";
    html += "        movementChart.update();";
 

    html += "        console.log(chartSizeOld);";
    html += "        console.log(moveData[moveData.length-1]);";
    // 🔔 Alarm Sound logic
   // html += "if (moveData.length > 0 && moveData[moveData.length-1] > 0) { alarmSound.play();   console.log('alarmSound  ----- play');}";
    html += "if ( moveData.length > chartSizeOld  && movemendDetected[movemendDetected.length-1] > 0 ) { alarmSound.play();   }";
    html += "else { alarmSound.pause(); alarmSound.currentTime = 0; }";

 /*   html += "        let lastMovement = moveData.length > 0 ? moveData[moveData.length-1] : 0;";
    html += "        if (lastMovement > 0) {";
//    html += "            if (!alarmPlaying) {";
     html += " console.log('Hallo, das ist eine Debug-Nachricht  ----- HOCUS POCKUS MUSIKOS');";
    html += "                alarmSound.play();";
    html += "                alarmPlaying = true;";
//   html += "            }";
    html += "        } else {";
    html += "            alarmSound.pause();";
    html +="                alarmSound.currentTime = 0;";
    html += "            alarmPlaying = false;";
    html += "        }";
  */  html += "    });";
    html += "} setInterval(updateChart, 2000);";

    html += "function setVolume(value) {";
    html += "    alarmSound.volume = value / 100;";
     html += "  console.log('Hallo, das ist eine Debug-Nachricht  ----- setVolume');";
    html += "   fetch('/setVolume?value=' + value);";  // Lautstärke an ESP senden
    html += "}";

    html += "</script>";
    html += "</head><body>";

    html += "<h1>Bewegungsprotokoll</h1>";
    html += "<canvas id='movementCanvas'></canvas>";
    html += "<script>";
    html += "let ctx = document.getElementById('movementCanvas').getContext('2d');";
    html += "let movementChart = new Chart(ctx, {";
    html += "    type: 'line', data: { labels: [], datasets: [";
    html += "        { label: 'Bewegung', borderColor: 'red', fill: false, data: [] },";
    html += "        { label: 'Stillstand', borderColor: 'blue', fill: false, data: [] }";
    html += "    ]}, options: { responsive: true }";
    html += "});";
    html += "</script>";

    html += "<h1>ESP32 WiFi Status</h1>";
    html += "<p><strong>IP-Adresse:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Signalstärke:</strong> " + String(rssi) + " dBm</p>";
    html += "<button onclick='window.location.reload();'>Manuell Aktualisieren</button>";
        html += "<p><a href='/config_wifi'>Go to WiFi Configuration</a></p>";


    html += "Lautstärke: <input type='range' min='0' max='100' value='" + String(alarmVolume) + "' oninput='setVolume(this.value)'><br>";

    html += "<h2>Einstellungen</h2>";
    html += "<form action='/settings_general' method='POST'>";
    html += "    Mindestbewegungsdauer (ms): <input type='number' name='minduration' value='" + String(minDetectionTime) + "'><br>";
    html += "    Alarm: <input type='checkbox' name='alarm' " + String(alarmEnabled ? "checked" : "") + "><br>";
    html += "    WhatsApp-Benachrichtigungen: <input type='checkbox' name='whatsapp' " + String(whatsappNotificationsEnabled ? "checked" : "") + "><br>";
    html += "    Alle Diagrammaufzeichnung: <input type='checkbox' name='record' " + String(recordAllDiagramData ? "checked" : "") + "><br>";
    html += "    <input type='submit' value='Speichern'>";
    html += "</form>";
    html += "<button onclick='requestNotificationPermission()'>Benachrichtigungen aktivieren</button>";

    html += "<h3>Sensor Konfiguration</h3>";
    html += "<form action='/settings_sensor' method='POST'>";
    html += "    Auflösung: <input type='number' name='resolution' value='" + String(sensorResolution) + "'><br>";
    html += "    Max. Bewegungsgate: <input type='number' name='maxMovingGate' value='" + String(maxMovingGate) + "'><br>";
    html += "    Max. Stillstandsgate: <input type='number' name='maxStationaryGate' value='" + String(maxStationaryGate) + "'><br>";
    html += "    <input type='submit' value='Speichern'>";
    html += "</form>";

    html += "<h3>Schwellenwerte</h3>";
    html += "<form action='/settings_sensor_advanced' method='POST'>";
    for (int i = 0; i < 9; i++) {
        int movingValue = movingParameters.values[i];
        int stationaryValue = stationaryParameters.values[i];
        html += "    Gate " + String(i + 1) + ": ";
        html += "    Bewegung: <input type='number' name='moving" + String(i) + "' value='" + String(movingValue) + "'> ";
        html += "    Stillstand: <input type='number' name='stationary" + String(i) + "' value='" + String(stationaryValue) + "'><br>";
    }
    html += "    <input type='submit' value='Speichern'>";
    html += "</form>";
    html += "<form action='/wallconfig' method='POST'><input type='submit' value='Erkennung hinter Wand'></form>";
    html += "<form action='/resetdefaults' method='POST'><input type='submit' value='Reset Default'></form>";

    html += "<h3>Erweitert</h3>";
    html += "<form action='/reset' method='POST'><input type='submit' value='Protokoll löschen'></form>";
    html += "<form action='/resetconfig' method='POST'><input type='submit' value='Sensor zurücksetzen'></form>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}



void handleSetVolume() {
    if (server.hasArg("value")) {
        alarmVolume = server.arg("value").toInt();
   //     Serial.print("Neue Lautstärke: ");
     //   Serial.println(alarmVolume);
    }
    server.send(200, "text/plain", "OK");
}


void handleGeneralSettings() {
    if (server.hasArg("minduration")) {
        minDetectionTime = server.arg("minduration").toInt();
    }
    alarmEnabled = server.hasArg("alarm");
    whatsappNotificationsEnabled = server.hasArg("whatsapp");
    recordAllDiagramData = server.hasArg("record");
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleSensorSettings() {
    if (server.hasArg("resolution")) {
        sensorResolution = server.arg("resolution").toInt();
        sensor.setResolution(sensorResolution);
    }
    if (server.hasArg("maxMovingGate")) {
        maxMovingGate = server.arg("maxMovingGate").toInt();
        sensor.setMaxMovingGate(maxMovingGate);
    }
    if (server.hasArg("maxStationaryGate")) {
        maxStationaryGate = server.arg("maxStationaryGate").toInt();
        sensor.setMaxStationaryGate(maxStationaryGate);
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleSettings_SensorAdvanced() {
    for (int i = 0; i < 9; i++) {
        if (server.hasArg("moving" + String(i))) {
            movingParameters.values[i] = server.arg("moving" + String(i)).toInt();
        }
        if (server.hasArg("stationary" + String(i))) {
            stationaryParameters.values[i] = server.arg("stationary" + String(i)).toInt();
        }
    }
    sensor.setGateParameters(movingParameters, stationaryParameters);
    server.sendHeader("Location", "/");
    server.send(303);
}

void resetToDefaults() {
    for (int i = 0; i < 9; i++) {
        movingParameters.values[i] = DEFAULT_MOVING_THRESHOLDS[i];
        stationaryParameters.values[i] = DEFAULT_STATIONARY_THRESHOLDS[i];
    }
    sensor.setGateParameters(movingParameters, stationaryParameters);
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleWallConfig() {
    sensor.setMaxMovingGate(4);
    sensor.setMaxStationaryGate(5);
    MyLD2410::ValuesArray wallMovingParams, wallStationaryParams;
    for (int i = 0; i < 9; i++) {
        wallMovingParams.values[i] = 30 + (i * 5);
        wallStationaryParams.values[i] = 25 + (i * 5);
    }
    sensor.setGateParameters(movingParameters, stationaryParameters);
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleResetConfig() {
    sensor.requestReset();
    logIndex = 0;
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleReset() {
    logIndex = 0;
    server.sendHeader("Location", "/");
    server.send(303);
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    sensorSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    pinMode(LED_PIN, OUTPUT);

    EEPROM.begin(EEPROM_SIZE);
    initWLAN();
    Serial.println("Verbunden!");
    Serial.print("ESP32 IP Address");
    Serial.println(WiFi.localIP());
    timeClient.begin();

    server.on("/", handleRoot);
    server.on("/logdata", handleLogData);
    server.on("/settings_sensor_advanced", HTTP_POST, handleSettings_SensorAdvanced);
    server.on("/settings_general", HTTP_POST, handleGeneralSettings);
    server.on("/settings_sensor", HTTP_POST, handleSensorSettings);
    server.on("/resetdefaults", HTTP_POST, resetToDefaults);
    server.on("/reset", HTTP_POST, handleReset);
    server.on("/wallconfig", handleWallConfig);
    server.on("/resetconfig", handleResetConfig);
  server.on("/setVolume", handleSetVolume);
  
    server.on("/config_wifi", handleConfigWifi);
    server.on("/save_wifi", HTTP_POST, handleSaveWifi);
    server.begin();

    webSocket.begin();
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {});

    sensor.setResolution(false);
    sensor.enhancedMode();
    printParameters();
}

void loop() {
    timeClient.update();
    server.handleClient();
    webSocket.loop();
    if ( wlan_ap_mode )dnsServer.processNextRequest();

    if (sensor.check() == MyLD2410::Response::DATA) {
        unsigned long currentTime = millis();
  //      printData();
        if (sensor.movingTargetDetected()) {
            if (currentTime - lastMovementTime > minDetectionTime) {
                int distance = sensor.movingTargetDistance();
                int stationaryDistance = sensor.stationaryTargetDistance();
                addMovement(distance, stationaryDistance, true);
             
               // notifyClients();
                lastMovementTime = currentTime;
                movementDetected = true;
            }
        } else {
                if (whatsappNotificationsEnabled && movementDetected) {
                  sendWhatsApp("Movement Alarm") ;   
                  //  sendSMS("!ALARM_MOVING!:" + String(sensor.movingTargetDistance()) + "cm / "+ String(sensor.stationaryTargetDistance()) + "cm") ;     //sendWhatsApp
                }
                movementDetected = false;

            static unsigned long prevLogAll = millis();
            if ( recordAllDiagramData && currentTime - prevLogAll > 10000 )
            {
              prevLogAll = millis();
                addMovement(sensor.movingTargetDistance(), sensor.stationaryTargetDistance(), false);
            }
        }
    }
  //  digitalWrite(LED_PIN, movementDetected ? HIGH : LOW);
}


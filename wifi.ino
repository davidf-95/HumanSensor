/*create new function for advanced wlan connection for my code.
seperate website for wlan the configurations. 
this site list all scanned ssids and allows to input two different wifi logins . 
a button save the logins in eeprom and connect to the wifi. 
if both wifi connection failed, we will create an accesPont allow user to edit configs.
*/


#define WIFI_SSID_1_ADDR 0
#define WIFI_PASS_1_ADDR 32
#define WIFI_SSID_2_ADDR 64
#define WIFI_PASS_2_ADDR 96


char ssid1[32], pass1[32], ssid2[32], pass2[32];

/*
void saveCredentials(const char* ssid1, const char* pass1, const char* ssid2, const char* pass2) {
  Serial.print("Save logins: ");
  Serial.println( ssid1 );
    EEPROM.put(WIFI_SSID_1_ADDR, ssid1);
    EEPROM.put(WIFI_PASS_1_ADDR, pass1);
    EEPROM.put(WIFI_SSID_2_ADDR, ssid2);
    EEPROM.put(WIFI_PASS_2_ADDR, pass2);
    EEPROM.commit();
}
*/
void saveCredentials(const char* s1, const char* p1, const char* s2, const char* p2) {
    Serial.println("Saving WiFi credentials to EEPROM...");

    char ssid1[32], pass1[32], ssid2[32], pass2[32];

    strncpy(ssid1, s1, sizeof(ssid1) - 1);
    strncpy(pass1, p1, sizeof(pass1) - 1);
    strncpy(ssid2, s2, sizeof(ssid2) - 1);
    strncpy(pass2, p2, sizeof(pass2) - 1);

    ssid1[sizeof(ssid1) - 1] = '\0';
    pass1[sizeof(pass1) - 1] = '\0';
    ssid2[sizeof(ssid2) - 1] = '\0';
    pass2[sizeof(pass2) - 1] = '\0';

  Serial.print("Save logins: ");
  Serial.println( ssid1 );

    EEPROM.put(WIFI_SSID_1_ADDR, ssid1);
    EEPROM.put(WIFI_PASS_1_ADDR, pass1);
    EEPROM.put(WIFI_SSID_2_ADDR, ssid2);
    EEPROM.put(WIFI_PASS_2_ADDR, pass2);
    EEPROM.commit();

    Serial.println("Credentials Saved!");
}

void loadCredentials() {  

    EEPROM.get(WIFI_SSID_1_ADDR, ssid1);
    EEPROM.get(WIFI_PASS_1_ADDR, pass1);
    EEPROM.get(WIFI_SSID_2_ADDR, ssid2);
    EEPROM.get(WIFI_PASS_2_ADDR, pass2);

      Serial.print("size ssid1: ");
  Serial.println( sizeof(ssid1) );
    Serial.print("SSID1: "); Serial.println(ssid1);

    // Ensure null termination
    ssid1[sizeof(ssid1) - 1] = '\0';
    pass1[sizeof(pass1) - 1] = '\0';
    ssid2[sizeof(ssid2) - 1] = '\0';
    pass2[sizeof(pass2) - 1] = '\0';

   // Check if SSID1 is valid (Basic validation)
    if (ssid1[0] == 0xFF || ssid1[0] == '\0') {
        Serial.println("EEPROM contains no valid WiFi credentials.");
        strcpy(ssid1, "");  // Reset to empty
        strcpy(pass1, "");
        strcpy(ssid2, "");
        strcpy(pass2, "");
    }

      Serial.print("size ssid1: ");
  Serial.println( sizeof(ssid1) );
    // Debug Output
    Serial.print("SSID1: "); Serial.println(ssid1);
    Serial.print("PASS1: "); Serial.println(pass1);
    Serial.print("SSID2: "); Serial.println(ssid2);
    Serial.print("PASS2: "); Serial.println(pass2);
}

bool connectWiFi(const char* ssid, const char* password) {
  Serial.print("Connect to wifi: ");
  Serial.println( ssid );
    WiFi.begin(ssid, password);
    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(500);
    }
    return false;
}

void startAccessPoint() {
   WiFi.mode(WIFI_AP);
    WiFi.softAP("ConfigAP", "12345678");

    // DNS-Server startet und leitet alle Anfragen auf die ESP32-IP um
    dnsServer.start(53, "*", WiFi.softAPIP());

    Serial.println("Access Point started!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    wlan_ap_mode = true;
}

void handleRedirect() {
    server.sendHeader("Location", "/config", true);
    server.send(302, "text/plain", "");
}


void handleConfigWifi() {
    String html = "<html><body><h2>WiFi Config</h2>";
    html += "<form method='post' action='/save_wifi'>";
    html += "SSID1: <input type='text' name='ssid1'><br>";
    html += "Pass1: <input type='password' name='pass1'><br>";
    html += "SSID2: <input type='text' name='ssid2'><br>";
    html += "Pass2: <input type='password' name='pass2'><br>";
    html += "<input type='submit' value='Save & Connect'>";
    html += "</form>";
    
    html += "<h3>Available Networks</h3><ul>";
    int numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks; i++) {
        html += "<li>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)</li>";
    }
    html += "</ul>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSaveWifi() {
   if (server.hasArg("ssid1") && server.hasArg("pass1") && server.hasArg("ssid2") && server.hasArg("pass2")) {
        String ssid1_str = server.arg("ssid1")+'\0';
        String pass1_str = server.arg("pass1");
        String ssid2_str = server.arg("ssid2");
        String pass2_str = server.arg("pass2");

        // Copy to character arrays and ensure null termination
        ssid1_str.toCharArray(ssid1, sizeof(ssid1));
        pass1_str.toCharArray(pass1, sizeof(pass1));
        ssid2_str.toCharArray(ssid2, sizeof(ssid2));
        pass2_str.toCharArray(pass2, sizeof(pass2));

      //  ssid1[sizeof(ssid1) - 1] = '\0';
        pass1[sizeof(pass1) - 1] = '\0';
        ssid2[sizeof(ssid2) - 1] = '\0';
        pass2[sizeof(pass2) - 1] = '\0';

        // Save to EEPROM
        saveCredentials(ssid1, pass1, ssid2, pass2);
    server.send(200, "text/html", "<html><body><h2>Saved! Rebooting...</h2></body></html>");
    delay(2000);
    ESP.restart();
        } else {
        server.send(400, "text/html", "<h2>Missing WiFi parameters!</h2>");
    }
}

void resetEEPROM() {
    Serial.println("Resetting EEPROM...");
    char empty[32] = {0};  // Fill with zeros
    saveCredentials(empty, empty, empty, empty);
}

void debugEEPROM() {
    Serial.println("Raw EEPROM Data:");
    for (int i = 0; i < 128; i++) {
        byte val = EEPROM.read(i);
        Serial.print(val, HEX);
        Serial.print(" ");
        if ((i + 1) % 16 == 0) Serial.println();
    }
}

void initWLAN()
{
 // resetEEPROM();    // only once
 //debugEEPROM();
    loadCredentials();
    
    if (!(connectWiFi(ssid1, pass1) || connectWiFi(ssid2, pass2))) {
        startAccessPoint();
    }
}
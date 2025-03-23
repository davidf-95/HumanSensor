
#include <Arduino.h>
String botToken = ":ABC--"; // Dein Bot-Token
String chatID = ""; // Deine Chat-ID

// twilio 
String accountSID = "";
String myNumber = "+41";
String authToken = "";

/*
void sendTelegramMessage(String message) {
  String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  http.end();
}
*/

void sendWhatsApp(String message) {
    static unsigned long prevPush  = 0;
   if ( millis() - prevPush > MIN_DELAY_NEXT_WHATSAPP_MSG)   {
    prevPush = millis();
  Serial.print("----Push sendWhatsApp  ---  t");
    
    String url = "https://api.twilio.com/2010-04-01/Accounts/" + accountSID + "/Messages.json";
    String postData = "To=whatsapp:" + urlEncode(myNumber) + "&From=whatsapp:"+ "&Body=" + urlEncode(message);


    HTTPClient http;
    http.begin(url);
    http.setAuthorization(accountSID.c_str(), authToken.c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST( postData );
    http.end();
  Serial.println();
  Serial.print("----Push sendWhatsApp  ---  t");
  Serial.println(millis() - prevPush);
  Serial.println( postData );
  Serial.println();
   }
}

void sendSMS(String message) {
    static unsigned long prevPush  = 0;
   if ( millis() - prevPush > MIN_DELAY_NEXT_WHATSAPP_MSG)   {
    prevPush = millis();
  Serial.print("----send sms  ---  t");
    
    String url = "https://api.twilio.com/2010-04-01/Accounts/" + accountSID + "/Messages.json";
    String postData = "To=" + urlEncode(myNumber) + "&From="+ urlEncode("+1xxxxx")+ "&Body=" + message;

    HTTPClient http;
    http.begin(url);
    http.setAuthorization(accountSID.c_str(), authToken.c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST( postData );
    http.end();
  Serial.println();
  Serial.print("----Push sendSMS  ---  t");
  Serial.println(millis() - prevPush);
  Serial.println( postData );
  Serial.println();
   }
}


void notifyClients() {
  static unsigned long prevPush  = millis();
  if ( millis() - prevPush > 5000) 
  {
  prevPush = millis();
    webSocket.broadcastTXT("motion_detected");
  Serial.println();
  Serial.println("----Push Call - notifyClients---");
 // sendWhatsApp(" Bewegung erkannt!");
 // sendTelegramMessage("🚨 Bewegung erkannt!");
  }
}


String encodePhoneNumber(String phone) {
  phone.replace("+", "%2B");
  return phone;
}


String urlEncode(const String &str) {
    String encoded = "";
    char c;
    char hex[4];
    for (size_t i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            sprintf(hex, "%%%02X", c);  // Convert to hex
            encoded += hex;
        }
    }
    return encoded;
}


/*

void handleRootPush() {
    String html = "<html><head>";
    html += "<script>";
    html += "var ws = new WebSocket('ws://' + window.location.hostname + ':81/');";
    html += "ws.onmessage = function(event) {";
    html += "if (event.data === 'motion') { sendPush(); }";
    html += "};";
    html += "if ('Notification' in window) {";
    html += "Notification.requestPermission().then(permission => {";
    html += "if (permission === 'granted') { new Notification('ESP32 Alarm aktiviert!'); }";
    html += "});}";
    html += "function sendPush() {";
    html += "if (Notification.permission === 'granted') {";
    html += "new Notification('🚨 Bewegung erkannt!', { body: 'Der Sensor hat eine Bewegung registriert.' });";
    html += "}}";
    html += "</script></head><body><h1>ESP32 WebSocket Push</h1></body></html>";
    server.send(200, "text/html", html);
}
*/
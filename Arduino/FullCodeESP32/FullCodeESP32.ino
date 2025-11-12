#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Output pins
const int LightGreen  = 19;
const int LightYellow = 20;
const int LightRed    = 18;
const int LightWhite  = 17;
const int analogPin   = A0;

bool firstSetup = false;
int state = 0;

Preferences prefs;
WebServer server(80);

String ssid = "";
String pass = "";

// Update LED states based on current selection
void updateLights() {
  digitalWrite(LightGreen,  state == 0 ? HIGH : LOW);
  digitalWrite(LightYellow, state == 1 ? HIGH : LOW);
  digitalWrite(LightRed,    state == 2 ? HIGH : LOW);
  digitalWrite(LightWhite,  state == 3 ? HIGH : LOW);

  String color;
  switch (state) {
    case 0: color = "Green"; break;
    case 1: color = "Yellow"; break;
    case 2: color = "Red"; break;
    case 3: color = "White"; break;
  }
  Serial.println("Light: " + color);
}

// Main page
void handleRoot() {
  String html;
  if (firstSetup) {
    html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><title>Wi-Fi Setup</title></head>
      <body>
        <h1>First-time Wi-Fi Setup</h1>
        <form action="/wifi" method="POST">
          SSID: <input name="ssid" type="text"><br>
          Password: <input name="pass" type="password"><br>
          <input type="submit" value="Save Wi-Fi">
        </form>
      </body>
      </html>
    )rawliteral";
  } else {
    html = "<!DOCTYPE html><html><head><title>Control Panel</title></head><body>";

    html += "<h1>Select Light Type</h1><form action=\"/setlight\" method=\"POST\">";
    html += "<input type=\"radio\" name=\"light\" value=\"0\" " + String(state == 0 ? "checked" : "") + "> Green<br>";
    html += "<input type=\"radio\" name=\"light\" value=\"1\" " + String(state == 1 ? "checked" : "") + "> Yellow<br>";
    html += "<input type=\"radio\" name=\"light\" value=\"2\" " + String(state == 2 ? "checked" : "") + "> Red<br>";
    html += "<input type=\"radio\" name=\"light\" value=\"3\" " + String(state == 3 ? "checked" : "") + "> White<br>";
    html += "<input type=\"submit\" value=\"Apply\"></form>";

    html += "<h1>Reset Wi-Fi</h1><form action=\"/resetwifi\" method=\"POST\">";
    html += "<input type=\"submit\" value=\"Delete Wi-Fi Credentials\"></form>";

    html += "</body></html>";
  }

  server.send(200, "text/html", html);
}

// Save Wi-Fi credentials
void handleWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    ssid = server.arg("ssid");
    pass = server.arg("pass");
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);

    Serial.println("Wi-Fi credentials saved:");
    Serial.println("SSID: " + ssid);
    Serial.println("PASS: " + pass);

    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><title>Wi-Fi Saved</title></head>
      <body>
        <h1>Wi-Fi saved successfully</h1>
        <form action="/reboot" method="POST">
          <input type="submit" value="Reboot Device">
        </form>
      </body>
      </html>
    )rawliteral";

    server.send(200, "text/html", html);
  } else {
    server.send(400, "text/html", "<h1>Error: Missing data</h1>");
  }
}

// Reset Wi-Fi credentials
void handleResetWiFi() {
  prefs.remove("ssid");
  prefs.remove("pass");
  Serial.println("Wi-Fi credentials deleted.");
  server.send(200, "text/html", "<h1>Wi-Fi credentials deleted</h1><p>Restart the device to reconfigure.</p>");
}

// Set light type from web interface
void handleSetLight() {
  if (server.hasArg("light")) {
    state = server.arg("light").toInt();
    prefs.putInt("state", state);
    Serial.println("Light selected: " + String(state));
    updateLights();
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/html", "<h1>Error: Invalid selection</h1>");
  }
}

// Set light type via remote HTTP GET/POST
void handleApiLight() {
  String method = server.method() == HTTP_GET ? "GET" : "POST";

  if (server.hasArg("state")) {
    int newState = server.arg("state").toInt();
    if (newState >= 0 && newState <= 3) {
      state = newState;
      prefs.putInt("state", state);
      updateLights();
      server.send(200, "application/json", "{\"status\":\"ok\",\"method\":\"" + method + "\",\"light\":" + String(state) + "}");
      Serial.println("Light changed via HTTP " + method + " to state: " + String(state));
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid state value\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing 'state' parameter\"}");
  }
}

// Query current light and voltage via HTTP GET
void handleApiStatus() {
  String color;
  switch (state) {
    case 0: color = "Green"; break;
    case 1: color = "Yellow"; break;
    case 2: color = "Red"; break;
    case 3: color = "White"; break;
  }

  int analogVolts = analogReadMilliVolts(analogPin);
  float voltage = analogVolts / 1000.0;

  String json = "{";
  json += "\"state\":" + String(state) + ",";
  json += "\"color\":\"" + color + "\",";
  json += "\"voltage\":" + String(voltage, 2);
  json += "}";

  server.send(200, "application/json", json);
}

// Reboot device
void handleReboot() {
  server.send(200, "text/html", "<h1>Restarting...</h1>");
  Serial.println("Restarting device...");
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Semaphore...");

  pinMode(LightGreen, OUTPUT);
  pinMode(LightYellow, OUTPUT);
  pinMode(LightRed, OUTPUT);
  pinMode(LightWhite, OUTPUT);
  analogReadResolution(12);

  prefs.begin("config", false);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  state = prefs.getInt("state", 0);
  updateLights();

  if (ssid == "") {
    firstSetup = true;
    WiFi.softAP("Semaphore", "123456");
    Serial.println("AP Mode Started: Semaphore");
    Serial.println("Connect to http://192.168.4.1");
  } else {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Connecting to Wi-Fi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi");
      Serial.println("IP Address: " + WiFi.localIP().toString());
    } else {
      Serial.println("\nFailed to connect to Wi-Fi");
    }
  }

  server.on("/", handleRoot);
  server.on("/wifi", HTTP_POST, handleWiFi);
  server.on("/resetwifi", HTTP_POST, handleResetWiFi);
  server.on("/setlight", HTTP_POST, handleSetLight);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/api/light", HTTP_ANY, handleApiLight);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.begin();
  Serial.println("Web Server Started.");
}

void loop() {
  server.handleClient();

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("status")) {
      int analogValue = analogRead(analogPin);
      int analogVolts = analogReadMilliVolts(analogPin);
      Serial.printf("Battery Voltage = %.2f V\n", analogVolts / 1000.0);
    }
  }
}

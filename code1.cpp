#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

#define SS 5
#define RST 14
#define DIO0 26

const char* ssid = "byp";
const char* password = "Byp12345678";

WebServer server(80);

String slave1OnTime = "-";
String slave1OffTime = "-";
String slave2OnTime = "-";
String slave2OffTime = "-";

unsigned long slave1StartMillis = 0;
unsigned long slave2StartMillis = 0;

String slave1Duration = "-";
String slave2Duration = "-";

bool slave1State = false;
bool slave2State = false;

// ---------------- TIME ----------------
String getTimeNow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "00:00:00";

  char timeString[9];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
  return String(timeString);
}

// ---------------- DURATION ----------------
String formatDuration(unsigned long durationMillis) {
  unsigned long seconds = durationMillis / 1000;

  int hrs = seconds / 3600;
  int mins = (seconds % 3600) / 60;
  int secs = seconds % 60;

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hrs, mins, secs);
  return String(buffer);
}

// ---------------- WEB PAGE ----------------
void handleRoot() {

  if (slave1State)
    slave1Duration = formatDuration(millis() - slave1StartMillis);

  if (slave2State)
    slave2Duration = formatDuration(millis() - slave2StartMillis);

  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<style>";
  html += "body{background:#111;color:white;text-align:center;font-family:Arial;}";
  html += "table{margin:auto;width:80%;font-size:22px;border-collapse:collapse;}";
  html += "th,td{padding:20px;border:1px solid white;}";
  html += ".on{color:green;}";
  html += ".off{color:red;}";
  html += "</style></head><body>";

  html += "<h1>LoRa Dashboard</h1>";

  html += "<table>";
  html += "<tr><th>Slave</th><th>Status</th><th>ON Time</th><th>OFF Time</th><th>Duration</th></tr>";

  html += "<tr><td>Slave1</td>";
  html += "<td class='" + String(slave1State ? "on'>ON" : "off'>OFF") + "</td>";
  html += "<td>" + slave1OnTime + "</td>";
  html += "<td>" + slave1OffTime + "</td>";
  html += "<td>" + slave1Duration + "</td></tr>";

  html += "<tr><td>Slave2</td>";
  html += "<td class='" + String(slave2State ? "on'>ON" : "off'>OFF") + "</td>";
  html += "<td>" + slave2OnTime + "</td>";
  html += "<td>" + slave2OffTime + "</td>";
  html += "<td>" + slave2Duration + "</td></tr>";

  html += "</table></body></html>";

  server.send(200, "text/html", html);
}

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  configTime(19800, 0, "pool.ntp.org");

  server.on("/", handleRoot);
  server.begin();

  //MOST IMPORTANT FIX
  SPI.begin(18, 19, 23, 5);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed");
    while (1);
  }

  Serial.println("Master Ready");
}

void loop() {

  server.handleClient();

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    // FIXED RECEIVE
    String received = LoRa.readString();
    received.trim();

    // FILTER
    if (!(received.startsWith("SLAVE1:") || received.startsWith("SLAVE2:"))) {
      Serial.println("Invalid Packet");
      return;
    }

    Serial.println("Received: " + received);

    unsigned long currentMillis = millis();

    // -------- SLAVE1 --------
    if (received.startsWith("SLAVE1:")) {

      String value = received.substring(7);

      if (value == "1" && !slave1State) {
        slave1State = true;
        slave1OnTime = getTimeNow();
        slave1StartMillis = currentMillis;
        Serial.println("Slave1 ON");
      }

      if (value == "0" && slave1State) {
        slave1State = false;
        slave1OffTime = getTimeNow();
        slave1Duration = formatDuration(currentMillis - slave1StartMillis);
        Serial.println("Slave1 OFF");
      }
    }

    // -------- SLAVE2 --------
    if (received.startsWith("SLAVE2:")) {

      String value = received.substring(7);

      if (value == "1" && !slave2State) {
        slave2State = true;
        slave2OnTime = getTimeNow();
        slave2StartMillis = currentMillis;
        Serial.println("Slave2 ON");
      }

      if (value == "0" && slave2State) {
        slave2State = false;
        slave2OffTime = getTimeNow();
        slave2Duration = formatDuration(currentMillis - slave2StartMillis);
        Serial.println("Slave2 OFF");
      }
    }
  }
}
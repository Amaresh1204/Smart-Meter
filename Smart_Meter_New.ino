#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include "ACS712.h"
#include <LiquidCrystal_I2C.h>
#include <ZMPT101B.h>

const char* ssid = "Gireesh";
const char* password = "12345678";

WebServer server(80);
int voltage, current, pf, frequency, power;
float f1, f2, totalUnitsConsumed = 0.0, pricePerUnit = 5.0;
unsigned long previousMillis = 0;
const long interval = 10000;

bool timerEnabled = true;
unsigned long timerStartMillis = 0;

String page = "";
ACS712 ACS(A0, 5.0, 4095, 185);
ZMPT101B zmpt(A1);
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Serial.begin(9600);
  delay(2000);
  lcd.begin(20, 4);
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.println("mA:min");
  lcd.setCursor(0, 1);
  lcd.print("mA:max");
  lcd.setCursor(0, 2);
  lcd.print("FF");

  ACS.autoMidPoint();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("\nConnected to Amaresh Server. IP Address: " + WiFi.localIP().toString());

  server.on("/", []() {
    page = "<html><head><title>Smart Energy Meter using IoT</title>";
    page += "<style type=\"text/css\">";
    page += "table{border-collapse: collapse;}th {background-color:  green ;color: white;}table,td {border: 4px solid black;font-size: x-large;";
    page += "text-align:center;border-style: groove;border-color: rgb(255,0,0);}</style>";
    page += "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css\">";
    page += "</head><body><center>";

    page += "<h1>Smart Energy Meter using IoT</h1><br><br><table style=\"width: 1200px;height: 450px;\"><tr>";
    page += "<th>Parameters</th><th>Value</th><th>Units</th></tr><tr><td>Voltage</td><td>" + String(voltage) + "</td><td>Volts</td></tr>";
    page += "<tr><td>Current</td><td>" + String(f1) + "</td><td>Amperes</td></tr><tr><td>Power Factor</td><td>" + String(pf) + "</td><td>XXXX</td>";
    page += "<tr><td>Power</td><td>" + String(f2) + "</td><td>Watts</td></tr><tr>";
    page += "<tr><td>Time Elapsed</td><td>" + formatTime(millis() - timerStartMillis) + "</td><td>HH:MM:SS</td></tr>";
    page += "</tr><tr><td>Frequency</td><td>" + String(frequency, 1) + "50</td><td>Hz</td></tr>";
    page += "</tr><tr><td>Price Per Unit</td><td>" + String(pricePerUnit) + " </td><td>Rs</td></tr>";
    page += "</tr><tr><td>Total Units Consumed</td><td>" + String(totalUnitsConsumed, 2) + "</td><td>Units</td></tr>";

    // Calculate power using the formula: P = VI
    power = voltage * f1 / 1000;

    // Calculate time elapsed
    unsigned long currentMillis = millis();
    unsigned long timeElapsed = currentMillis - timerStartMillis;

    if (timerEnabled) {
      // Calculate total units consumed using the formula: Energy (kWh) = Power (kW) * Time (hours)
      totalUnitsConsumed += (power / 1000.0) * (timeElapsed / 3600000.0);
    }

    // Calculate total cost using the formula: Cost = Total Units Consumed * Price Per Unit
    float totalCost = totalUnitsConsumed * pricePerUnit;

    page += "<tr><td>Total Cost</td><td>" + String(totalCost, 2) + " </td><td>Rs</td></tr>";
    page += "</table>";

    // Display the current and power speedometers side by side below the table
    page += "<div style=\"display:flex; justify-content: space-between;\">";
    page += "<div style=\"flex:1;\"><h2>Current</h2><canvas id=\"currentGauge\" width=\"200\" height=\"200\"></canvas></div>";
    page += "<div style=\"flex:1;\"><h2>Power</h2><canvas id=\"powerGauge\" width=\"200\" height=\"200\"></canvas></div>";
    page += "</div>";

    page += "<meta http-equiv=\"refresh\" content=\"3\">";

    // JavaScript code for drawing the gauges
    page += "<script>";
    page += "function drawGauge(canvasId, value, maxValue, label) {";
    page += "var canvas = document.getElementById(canvasId);";
    page += "var context = canvas.getContext('2d');";
    page += "context.clearRect(0, 0, canvas.width, canvas.height);";
    page += "context.beginPath();";
    page += "context.arc(canvas.width / 2, canvas.height / 2, canvas.width / 2 - 10, 0, 2 * Math.PI, false);";
    page += "context.lineWidth = 10;";
    page += "context.strokeStyle = '#ddd';";
    page += "context.stroke();";
    page += "context.beginPath();";
    page += "context.arc(canvas.width / 2, canvas.height / 2, canvas.width / 2 - 10, 1.5 * Math.PI, (value / maxValue) * 2 * Math.PI - 0.5 * Math.PI, false);";
    page += "context.lineWidth = 10;";
    page += "context.strokeStyle = '#4CAF50';";
    page += "context.stroke();";
    page += "context.font = '20px Arial';";
    page += "context.fillStyle = '#333';";
    page += "context.textAlign = 'center';";
    page += "context.fillText(value, canvas.width / 2, canvas.height / 2 + 10);";
    page += "context.fillText(label, canvas.width / 2, canvas.height / 2 + 40);";
    page += "}";
    page += "drawGauge('currentGauge', " + String(f1) + ", 20, 'Amps');";
    page += "drawGauge('powerGauge', " + String(f2) + ", 500, 'Watts');";
    page += "</script>";

    // Add styling to the reset button
    page += "<form action=\"/resetVirtual\" method=\"post\">";
    page += "<input type=\"submit\" class=\"btn btn-danger\" value=\"Reset Timer\">";
    page += "</form>";

    page += "</center></body></html>";

    server.send(200, "text/html", page);
  });

  server.on("/reset", HTTP_POST, resetTimer);
  server.on("/resetVirtual", HTTP_POST, resetTimer);

  server.begin();
}

void loop() {
  f1 = ACS.mA_AC();
  f2 = ACS.mA_AC_sampling();
  voltage = 230;
  frequency = 50;

  server.handleClient();

  lcd.setCursor(4, 2);
  lcd.println(f1 / f2, 2);

  delay(interval);
}

void resetTimer() {
  // Reset the timer and total units consumed when the button is pressed (physical or virtual)
  timerStartMillis = millis();
  totalUnitsConsumed = 0.0;
  server.send(200, "text/plain", "Timer Reset");
}

// Function to format time in HH:MM:SS format
String formatTime(unsigned long milliseconds) {
  unsigned long seconds = milliseconds / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  return String(hours) + ":" + String(minutes % 60) + ":" + String(seconds % 60);
}

#include <Sgp4.h>
#include <Streaming.h>
#include <time.h>
#include <WiFi.h>

// Satellite config (replace TLE with current CelesTrak data when ready)
Sgp4 sat;
char satName[] = "METEOR-M2 4";
char tleLine1[] = "1 59051U 24039A   26330.60223462 -.00000013  00000+0  13910-4 0  9999";
char tleLine2[] = "2 59051  98.7261 288.0371 0008071  63.5511 296.6494 14.22444475142353";
          
// WiFi config
const char* ssid = "SSID_HERE";
const char* password = "PASSWORD_HERE";

// NTP server config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

static int mode = 0; // 0 - Normal Operational, 1 - Maintenance mode, 2 - (Planned) SatNOGS może coś kiedyś wymyśle czy coś, 3 - Safe mode
bool isThrottled = false;

void calibrateExpert() {
  // niepojem, trzeba czekać po silniczki
}

void maintenanceMode() {
  static int wasrun = 0;

  if (wasrun == 0) {
    Serial.println("WARNING: Maintenance Mode is Activated, all user actions are reset."); Serial.println("");
    wasrun = wasrun + 1;
  }

  calibrateExpert();
}

void calibrateUserland() {
  // tutaj tak samo jak w calibrateExpert()
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.print("Wi-Fi disconnected, reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
  }
}

void syncTimeNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Syncing NTP time");

  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < 1700000000 && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  if (now >= 1700000000) {
    Serial.print("\nTime synced! UNIX: ");
    Serial.println(now);
  } else {
    Serial.println("\nNTP sync timed out.");
  }
}

void safeMode() {
  Serial.println("Entering Safe Mode...");
  mode = 3
  // tu trzeba zmniejszyć mhz procka i zmniejszyć wifi aby zaoszczedzić cpu
}

void throttleProtection() {
  //float temp_c = temperatureRead();
  float temp_c = 70.00;
  if (temp_c >= 70.00) {
    Serial.println("Thermal Protection Tripped. Oh great heavens CPU OVERHEATS! ");
    isThrottled = true;
  }

  if (isThrottled == true) {
    safeMode();
  }
}

void sateliteTrack() {
  time_t t = time(nullptr);
  if (t < 1700000000) {
    Serial.println("System time not set yet");
    return;
  }

  unsigned long now = static_cast<unsigned long>(t);
  sat.findsat(now);
  float temp_c = temperatureRead();

  Serial << "UNIX Time: " << now << " | Az: " << sat.satAz << " st." << " | El: " << sat.satEl << " st." << " Current Target: " << satName << " [|] CPU Temp: " << temp_c << " °C" << endl;
}

void connectWiFiWithReset() {
  WiFi.onEvent(onWiFiEvent);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(500);

  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    syncTimeNTP();
  } else {
    Serial.print("\nFailed to connect. Status: ");
    Serial.println(WiFi.status());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  connectWiFiWithReset();

  sat.site(LANGTITUDE, LONGITUDE, ALTITUDE); //set site latitude[°], longitude[°] and altitude[m]
  sat.init(satName, tleLine1, tleLine2);
}

void loop() {
  throttleProtection();

  if (mode == 1) {
    maintenanceMode();
    delay(2000);
  } else if (mode == 2) {
    // Place for mode 2
  } else if (mode == 3) {
    safeMode();
  } else {
    sateliteTrack();
    delay(2000);
  }
}

// TODO:
// 1. Call throttleProtection() each loop and gate rotor/motor commands when isThrottled is true.
// 2. Return early from maintenanceMode() once calibration is complete and set mode = 0.
// 3. Implement rotor/antenna positioning.

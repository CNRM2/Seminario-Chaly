#include "HX711.h"
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>

#define LOADCELL_DOUT_PIN  34
#define LOADCELL_SCK_PIN  33

#define WIFI_SSID "iPhone"
#define WIFI_PASSWORD "12345678"
#define API_KEY "AIzaSyA03Uw6_d7vZfrAP7RgtDt36gAAp5pMrQg"
#define DATABASE_URL "https://gasrt-558de-default-rtdb.firebaseio.com"
#define USER_EMAIL "jvaldezcazares@gmail.com"
#define USER_PASSWORD "Al_175423"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
HX711 scale;

float calibration_factor = 22050; //-7050 worked for my 440lb max scale setup
float max_weight = 16.5; // Maximo peso en kg (incluyendo el tanque)
float tank_weight = 6.0; // Peso del tanque vacío en kg

unsigned long lastSendDataMillis = 0;
const unsigned long SEND_DATA_INTERVAL = 3000; // 3 segundos en milisegundos

void setup() {
  Serial.begin(115200);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado con IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Cliente Firebase v%s\n\n", FIREBASE_CLIENT_VERSION);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
}

void loop() {
  unsigned long currentMillis = millis();

  scale.set_scale(calibration_factor);

  Serial.print("Reading: ");
  float weight = scale.get_units(0); // Obtenemos el peso en kg
  float net_weight = weight - tank_weight; // Restamos el peso del tanque vacío
  Serial.print(net_weight, 3); // Mostramos el peso neto en el monitor serial
  Serial.print(" kg");

  // Convertimos el peso neto a porcentaje
  float percentage = (net_weight / (max_weight - tank_weight)) * 100.0;
  Serial.print(" | Percentage: ");
  Serial.print(percentage, 3); // Mostramos el porcentaje en el monitor serial
  Serial.println();
  Serial.print(calibration_factor, 3);

  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if (temp == '-' || temp == 'z')
      calibration_factor -= 10;
  }

  if (Firebase.ready() && (currentMillis - lastSendDataMillis >= SEND_DATA_INTERVAL || lastSendDataMillis == 0)) {
    lastSendDataMillis = currentMillis;
    
    sendDataToFirebase(percentage);
  }
}

void sendDataToFirebase(float percentage) {
  // Se envía el porcentaje al atributo sensor_data bajo el dispositivo con ID -Ny1JX-I4AsZQU0nkvGc
  Firebase.setFloat(fbdo, "users/IdmNGYT7vgSn67VmTtPUsKtqxKW2/device/-Ny1JX-I4AsZQU0nkvGc/sensor_data", percentage);
}

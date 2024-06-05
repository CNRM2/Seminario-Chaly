#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>

const int sensorPin = 32; // Pin analógico conectado al sensor de gas
const int valvulaPin = 2;

int gasValue = 0;
float gasPercentage = 0.0;
boolean valvula1 = true;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastGasCheckTime = 0;
unsigned long lastSendDataMillis = 0;
const unsigned long gasCheckInterval = 1000; // Intervalo de verificación de gas en milisegundos
const unsigned long SEND_DATA_INTERVAL = 1000; // 15 segundos en milisegundos

// Define tus credenciales del proyecto Firebase
#define WIFI_SSID "Megacable_KQURrFv"
#define WIFI_PASSWORD ""
#define API_KEY ""
#define DATABASE_URL ""
#define USER_EMAIL ""
#define USER_PASSWORD ""

void setup() {
  Serial.begin(115200);
  pinMode(valvulaPin, OUTPUT);

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
  if (Firebase.ready()) {
    unsigned long currentMillis = millis();

    // Verifica el gas cada cierto tiempo
    if (currentMillis - lastGasCheckTime >= gasCheckInterval || lastGasCheckTime == 0) {
      lastGasCheckTime = currentMillis;

      gasValue = analogRead(sensorPin);
      gasPercentage = map(gasValue, 0, 4095, 0, 100); // Mapea el valor del sensor al rango de porcentaje

      Serial.print("Valor de gas: ");
      Serial.print(gasValue);
      Serial.print(", Porcentaje de gas: ");
      Serial.println(gasPercentage);

      if (gasPercentage > 30.0) {
        valvula1 = false;
        Serial.println("SE DETECTO GAS!!!!");
      } else {
        valvula1 = true;
        Serial.println("Valvula Desactivada");
      }
    }

    // Control de la válvula y el buzzer
    if (valvula1) {
      digitalWrite(valvulaPin, LOW);
    } else {
      digitalWrite(valvulaPin, HIGH);
    }
    // Envía datos a Firebase cada cierto tiempo
    if (currentMillis - lastSendDataMillis >= SEND_DATA_INTERVAL || lastSendDataMillis == 0) {
      lastSendDataMillis = currentMillis;
      sendDataToFirebase(gasPercentage, gasPercentage > 30);
    }
  }
}

void sendDataToFirebase(int gasValue, bool ledStatus) {
  Firebase.setInt(fbdo, "/users/IdmNGYT7vgSn67VmTtPUsKtqxKW2/device/-Ny1JGi_Cl-xzrP_fTMG/sensor_data", gasValue);
  Firebase.setBool(fbdo, "/users/IdmNGYT7vgSn67VmTtPUsKtqxKW2/device/-Ny1JGi_Cl-xzrP_fTMG/led_status", ledStatus);
}

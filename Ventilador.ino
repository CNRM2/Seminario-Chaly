#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "wifiLabGral";
const char* password = "";
const char* serverAddress = "https://gasrt.000webhostapp.com/scriptESP.php"; // Reemplaza con la dirección de tu script PHP
const int ledPin = 2; // Pin donde está conectado el LED integrado

unsigned long lastRequestTime = 0; // Variable para almacenar el tiempo de la última solicitud
const unsigned long requestInterval = 1000; // Intervalo entre solicitudes (en milisegundos)

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(ledPin, OUTPUT);
  
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
}

void loop() {
  // Obtener el tiempo actual
  unsigned long currentMillis = millis();

  // Verificar si ha pasado el intervalo de tiempo desde la última solicitud
  if (currentMillis - lastRequestTime >= requestInterval) {
    Serial.println("\nExecuting HTTP request...");

    HTTPClient http;

    // Configurar la dirección del servidor y el puerto
    http.begin(serverAddress);

    // Realizar la solicitud HTTP GET y esperar la respuesta
    int httpCode = http.GET();

    // Verificar el código de estado de la respuesta
    if (httpCode > 0) {
      // Si la respuesta fue exitosa, obtener la respuesta JSON
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);

      // Crear un objeto DynamicJsonDocument para analizar la respuesta JSON
      DynamicJsonDocument jsonDoc(1024);
      deserializeJson(jsonDoc, payload);

      // Obtener el valor del campo "command"
      String command = jsonDoc["command"];

      // Verificar el valor del campo "command" y actuar en consecuencia
      if (command == "turn_off") {
        Serial.println("Turning off the integrated LED...");
        digitalWrite(ledPin, LOW); // Apagar el LED
      } else if (command == "turn_on") {
        Serial.println("Turning on the integrated LED...");
        digitalWrite(ledPin, HIGH); // Encender el LED
      } else {
        Serial.println("Unknown command received.");
      }
    } else {
      // Si la solicitud falló, mostrar el código de error
      Serial.print("HTTP request failed with error code: ");
      Serial.println(httpCode);
    }

    // Cerrar la conexión HTTP
    http.end();

    // Actualizar el tiempo de la última solicitud
    lastRequestTime = currentMillis;
  }
}

#include "WiFi.h"
#include <esp_now.h>

int itemIdx;

#define CHANNEL 1
enum Role { Primary, Secondary };
Role role = Secondary; // Selecciona el rol de este dispositivo
esp_now_peer_info_t peer;

unsigned long peer_ping_interval = 1 * 1000; // 1 segundo
unsigned long peer_ping_lasttime = 0;
unsigned long peer_check_interval = 3 * 1000; // 3 segundos
unsigned long peer_last_packet_time = 0;

void setup() {
  Serial.begin(115200); // Inicializa el puerto serie

  // Crea una tarea para manejar ESP-NOW
  xTaskCreate(espnowTask,
              "espnowTask",
              4096,
              NULL,
              1,
              NULL);

  Serial.println("Configuración completa. Escribe un mensaje para enviar:");
}

void loop() {
  // Revisa si hay datos disponibles en el puerto serie
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n'); // Lee el mensaje hasta el salto de línea
    message.trim(); // Elimina espacios en blanco
    if (message.length() > 0) {
      sendSerialMessage(message); // Envía el mensaje si no está vacío
    }
  }
}

// Función para enviar un mensaje desde el puerto serie al peer
void sendSerialMessage(String message) {
  if (*peer.peer_addr != NULL) { // Verifica si hay un peer conectado
    if (sendMsgToPeer(peer.peer_addr, message)) {
      Serial.println("Mensaje enviado: " + message);
    } else {
      Serial.println("Error al enviar el mensaje.");
    }
  } else {
    Serial.println("No hay peer conectado.");
  }
}

// Tarea de ESP-NOW
void espnowTask(void *pvParameters) {
  vTaskDelay(1000); // Espera 1 segundo

  if (role == Primary) {
    Serial.println("[Modo Primario]");
    WiFi.mode(WIFI_STA); // Configura el modo WiFi como STA
  } else {
    Serial.println("[Modo Secundario]");
    WiFi.mode(WIFI_AP_STA); // Configura el modo WiFi como AP+STA
    configDeviceAP(); // Configura el AP
  }

  InitESPNow(); // Inicializa ESP-NOW
  esp_now_register_recv_cb(OnDataRecv); // Registra la función de callback para recibir datos

  while (1) {
    if (role == Primary) {
      ScanForPeer(); // Escanea para encontrar peers si el rol es Primario
    }
    sendPing(); // Envía un ping al peer
    checkLastPacket(); // Verifica el tiempo desde el último paquete recibido
    vTaskDelay(50); // Espera 50ms antes de repetir el bucle
  }
}

// Configura el dispositivo como AP
void configDeviceAP() {
  String Prefix = "Peer:";
  String Mac = WiFi.macAddress();
  Serial.print("AP MAC: ");
  Serial.println(Mac);

  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    Serial.println("Error al configurar el AP.");
    ESP.restart();
  } else {
    Serial.println("Configuración de AP exitosa. Emitiendo con AP: " + String(SSID));
  }
}

// Inicializa ESP-NOW
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == 0) {
    Serial.println("Inicialización de ESPNow exitosa");
    Serial.println("- ESP-NOW LISTO -");
  } else {
    Serial.println("Error en la inicialización de ESPNow");
    ESP.restart();
  }
}

// Función de callback cuando se reciben datos
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (role == Secondary && *peer.peer_addr == NULL) {
    char macAddr[6];
    sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    memcpy(&peer.peer_addr, macAddr, 6);
    peer.channel = CHANNEL;
    peer.encrypt = 0;

    const esp_now_peer_info_t *peerNode = &peer;
    esp_err_t addStatus = esp_now_add_peer(peerNode);
    if (addStatus == 0) {
      Serial.println("- Conectado -");
      sendMsgToPeer(peer.peer_addr, "¡Hola Peer Primario!");
    }
  }

  String receivedString = (char*)data;
  receivedString.trim();
  if (receivedString.length() > 0) {
    Serial.println("Mensaje recibido: " + receivedString);
  }

  peer_last_packet_time = millis(); // Actualiza el tiempo del último paquete recibido
}

// Verifica si el peer está conectado y envía un mensaje
void checkPeerToSendMsg(String msg) {
  msg.trim();
  if (msg.length() == 0) {
    return;
  }

  if (*peer.peer_addr == NULL) {
    Serial.println("peer.peer_addr == NULL");
    return;
  }

  bool sentResult = sendMsgToPeer(peer.peer_addr, msg);

  if (sentResult) {
    Serial.println("Mensaje enviado: " + msg);
  }
}

// Envía un mensaje al peer
bool sendMsgToPeer(uint8_t *peer_addr, String msg) {
  byte byteMsg[msg.length() + 1];
  msg.getBytes(byteMsg, msg.length() + 1);

  esp_err_t result = esp_now_send(peer_addr, (uint8_t *)byteMsg, sizeof(byteMsg));
  return !result;
}

// Escanea para encontrar peers
void ScanForPeer() {
  if (*peer.peer_addr != NULL) {
    return;
  }

  int8_t scanResults = WiFi.scanNetworks();
  if (scanResults > 0) {
    for (int i = 0; i < scanResults; ++i) {
      String SSID = WiFi.SSID(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      vTaskDelay(10);

      if (SSID.indexOf("Peer") == 0) {
        int mac[6];
        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])) {
          char macAddr[6];
          sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

          esp_now_peer_info_t temp;
          memcpy(&temp.peer_addr, macAddr, 6);

          if (esp_now_is_peer_exist(temp.peer_addr)) {
            Serial.println("¡Peer ya existe!");
            continue;
          }

          memcpy(&peer.peer_addr, macAddr, 6);
          peer.channel = CHANNEL;
          peer.encrypt = 0;

          const esp_now_peer_info_t *peerNode = &peer;
          if (esp_now_add_peer(peerNode) != ESP_OK) {
            Serial.println("Error al agregar el peer");
            continue;
          }

          Serial.println("- Conectado -");
          checkPeerToSendMsg("¡Hola!");
        }
      }
    }
  }

  WiFi.scanDelete();
  vTaskDelay(1000);
}

// Envía un ping al peer
void sendPing() {
  if (millis() > peer_ping_lasttime + peer_ping_interval) {
    if (*peer.peer_addr != NULL) {
      byte pingByteMsg[1];
      String pingMsg = " ";
      pingMsg.getBytes(pingByteMsg, pingMsg.length());
      esp_now_send(peer.peer_addr, (uint8_t *)pingByteMsg, sizeof(pingByteMsg));
    }
    peer_ping_lasttime = millis();
  }
}

// Verifica el tiempo desde el último paquete recibido
void checkLastPacket() {
  if (*peer.peer_addr != NULL) {
    unsigned long sinceLastPacket = millis() - peer_last_packet_time;
    if (sinceLastPacket > peer_check_interval) {
      delPeer();
      *peer.peer_addr = NULL;
      Serial.println("- Peer desconectado! -");
    }
  }
}

// Elimina el peer
void delPeer() {
  esp_now_del_peer(peer.peer_addr);
}

#include <WiFi.h>
#include "global.hpp"

const char* ssid = "V-LINK";
const char* password = "";

WiFiClient client;
bool is_wifi_connected = false; 
bool is_tcp_connected = false; 

void setupWifi() {
    Serial.begin(115200); 
    Serial.println("🔌 Connessione alla Vgate Wi-Fi...");
    WiFi.begin(ssid, password);
}

void checkWifiStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!is_wifi_connected) {
            Serial.println("\n✅ Connesso al Wi-Fi Vgate");
            is_wifi_connected = true;
        }
    } else {
        if (is_wifi_connected) {
            Serial.println("❌ Wi-Fi Disconnesso.");
            is_wifi_connected = false;
            is_tcp_connected = false; 
        }
    }
}

// questa funzione e' bloccante
// possiamo considerare di fare diversamente
int checkConnection() {
    if (!is_wifi_connected) {
        is_tcp_connected = false;
        return -1;
    }
    
    if (client.connected()) {
        is_tcp_connected = true;
        return 0; 
    }

    if (is_tcp_connected) {
        is_tcp_connected = false;
        Serial.println("⚠️ Connessione TCP caduta. Riprovo...");
    }
    
    Serial.print("Tentativo TCP...");

    if (client.connect("192.168.0.10", 35000)) {
        Serial.println("✅ Connesso via TCP. Inizializzo OBD.");
        is_tcp_connected = true;
        
        // Inizializzazione dongle OBD
        client.print("ATZ\r");
        client.print("ATE0\r");
        client.print("ATL0\r");
        client.print("ATSP0\r");
        return 0;
    }

    Serial.println("❌ TCP fallito.");
    return -1;
}

float sendOBDCommand() {
    int current_id;

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current_id = ui_index;
        xSemaphoreGive(xUIMutex);
    } else {
        current_id = ui_index;
    }

    // se non sono connesso allora returno -1
    if (!is_tcp_connected) return -1; 
    
    int resbytes = OBDScreens[current_id].resbytes;
    if (resbytes > 4 || resbytes < 1) return -1;

    client.setTimeout(200); 

    // altrimenti parto conla richiesta
    String command = "01";
    command += OBDScreens[current_id].obd_code;
    command += "\r";
    client.print(command);

    // Aspetta la risposta fino al carattere de prompt '>'
    String response = client.readStringUntil('>'); 
    
    // se mi arriva una risposta fetcho e estrapolo di dati
    if (response.length() == 0) return -1;

    response.replace(" ", "");
    response.toUpperCase();

    int index = response.indexOf(String("41") + OBDScreens[current_id].obd_code);
    if (index == -1) return -1;

    if (response.length() < index + 4 + resbytes * 2) return -1;

    long value = 0;
    for (int i = 0; i < resbytes; i++) {
        String byteStr = response.substring(index + 4 + i * 2, index + 4 + i * 2 + 2);
        int byteVal = strtol(byteStr.c_str(), nullptr, 16);
        value = (value << 8) | byteVal;
    }

    // interpreto il dato come richiesto
    return OBDScreens[current_id].interpretation(value);
}
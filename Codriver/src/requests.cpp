    #include <Wifi.h>
    #include "global.hpp"

    const char* ssid = "V-LINK";
    const char* password = "";

    WiFiClient client;

    // funzione per connettersi automaticamente

    void setupWifi() {
        // mi connetto alla rete obd ( macchina su stato on, non serve accesa)
        // praticamente genera una rete privata

        Serial.println("🔌 Connessione alla Vgate Wi-Fi...");
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\n Connesso al Wi-Fi Vgate");

        //stabilizzo la connessione alla obd con il suo ip privato

        Serial.println("🔌 Connessione TCP alla Vgate...");
        if (!client.connect("192.168.0.10", 35000)) {
            Serial.println("❌ Connessione TCP fallita");
        } else {
            Serial.println("✅ Connesso alla Vgate via TCP");

            // Inizializzazione dongle OBD
            // servono per dare il primo comando di start
            client.print("ATZ\r");
            delay(1000);
            client.print("ATE0\r");   
            delay(100);
            client.print("ATL0\r");  
            delay(100);
            client.print("ATSP0\r");  
            delay(1000); // qui dovrebbe avere tempo di inizializzarsi
        }
    }

    // funzione per controllare la connessione e in caso riconnettersi
    int checkConnection() {
        if (client.connected()) return 0; // già connesso

        Serial.println("⚠️ Disconnesso, provo a riconnettere...");
        int tries = 0;
        const int maxTries = 10;

        while (!client.connected() && tries < maxTries) {
            Serial.print("Tentativo di connessione ");
            Serial.println(tries + 1);

            if (client.connect("192.168.0.10", 35000)) {
                Serial.println("✅ Connesso al dongle OBD");
                // reinizializza OBD
                client.print("ATZ\r");
                client.print("ATE0\r");
                client.print("ATL0\r");
                client.print("ATSP0\r");
                return 0;
            }

            tries++;
            delay(500); // piccola pausa tra i tentativi
        }

        Serial.println("❌ Impossibile connettersi dopo 10 tentativi");
        return -1;
    }

    // questa funzione invia il comando e restituisce la risposta sotto forma 
    // di valore convertito in long

    long sendOBDCommand() {

        int current_id;

        if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            current_id = ui_index;

            xSemaphoreGive(xUIMutex);
        } else {
            current_id = ui_index;
        }


        // controlla la connessione e prova a ristabilirla
        int resbytes = OBDScreens[current_id].resbytes;

        if (checkConnection() != 0) return -1; 
        if (resbytes > 4 || resbytes < 1) return -1;

        // invia la richiesta
        String command = "01";
        command += OBDScreens[current_id].obd_code;
        command += "\r";
        client.print(command);
        // delay(200);

        String response = "";
        unsigned long start = millis();
        // timeout di 50ms per ricevere la risposta
        // FORSE ECCESSIVO DA RIVEDERE
        while (millis() - start < 200) {
            while (client.available()) {
                char c = client.read();
                Serial.print("Tempo di risposta: ~");
                Serial.println(millis() - start);
                if (c != '\r' && c != '\n' && c != '>') response += c;
            }
        }

        response.replace(" ", "");
        response.toUpperCase();

        int index = response.indexOf(String("41") + OBDScreens[current_id].obd_code);
        if (index == -1) return -1;

        // verifica che ci siano abbastanza caratteri
        if (response.length() < index + 4 + resbytes * 2) return -1;

        long value = 0;
        for (int i = 0; i < resbytes; i++) {
            String byteStr = response.substring(index + 4 + i * 2, index + 4 + i * 2 + 2);
            int byteVal = strtol(byteStr.c_str(), nullptr, 16);
            value = (value << 8) | byteVal;
        }

        return OBDScreens[current_id].interpretation(value);
    }


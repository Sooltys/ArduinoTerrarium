# 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
# 2 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
# 3 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2

# 5 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
# 6 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2

# 8 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
# 9 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2

# 11 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2


# 12 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
// czujjnik temperatury i wilgoci
# 27 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
int ostatniStanPrzyciskPlus = 0x1;
int ostatniStanPrzyciskMinus = 0x1;
int temperaturaUstawiona = 30;
int staraTemperaturaUstawiona = temperaturaUstawiona;
float wilgotnosc;
float temperatura;
int trybWyswietlacza = 0;
boolean zmianaTrybu = false;
int ostatniStanPrzyciskuZmiany = 0x1;

// inicjalizacja czujnika temp i wilg
DHT dht(2, 11 /**< DHT TYPE 11 */);

// inicjalizacja wyświetlacza 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// zmienne do odliczania millis()
unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzas[] = {-10000, -60000, -5000};
unsigned long roznicaCzasu[] = {0, 0, 0};

// zmienne sieci WiFi
char ssid[] = "FunBox2-EF66"; // SSID sieci WiFi
char pass[] = "NIEMAHASLA"; // haslo sieci WiFi
//int keyIndex = 0;                   // dla sieci zabezpieczonej WEP
int status = WL_IDLE_STATUS;

// zmienne łączenia z serwerem
WiFiClient client;
char SERVER[] = "www.pj41491.zut.edu.pl";
int HTTP_PORT = 80;

void setup() {
    Serial.begin(9600);

    pinMode(3, 0x2);
    pinMode(6, 0x2);
    pinMode(5, 0x2);
    pinMode(15, 0x1);
    pinMode(14, 0x1);
    pinMode(16, 0x1);
    digitalWrite(15, 0x1);
    digitalWrite(14, 0x0);
    digitalWrite(16, 0x1);
    pinMode(17, 0x1);

    dht.begin();

    lcd.begin();
    lcd.backlight();
    lcd.clear();

    connectWiFi();

    // ustawienie watchdog'a
    wdt_enable(
# 82 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
              9
# 82 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                     );
}

void loop() {
    // reset licznika watchdog'a
    
# 87 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
   __asm__ __volatile__ ("wdr")
# 87 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
              ;

    // wykorzystanie millis() zamiast delay() aby nie zatrzymywać całego mikrokontrolera
    aktualnyCzas = millis();

    roznicaCzasu[2] = aktualnyCzas - zapamietanyCzas[2];
    if (roznicaCzasu[2] >= 5000UL) {
        zapamietanyCzas[2] = aktualnyCzas;
        wilgotnosc = dht.readHumidity();
        temperatura = dht.readTemperature();
    }

    roznicaCzasu[0] = aktualnyCzas - zapamietanyCzas[0];
    if (roznicaCzasu[0] >= 10000UL) {
        zapamietanyCzas[0] = aktualnyCzas;
        wyswietlDaneNaLCD();
        if(temperaturaUstawiona == staraTemperaturaUstawiona) {
            temperaturaUstawiona = pobierzTemperature();
        }
    }
    // reset licznika watchdog'a
    
# 108 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
   __asm__ __volatile__ ("wdr")
# 108 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
              ;

    if(temperaturaUstawiona > temperatura) {
        digitalWrite(15, 0x0);
        digitalWrite(16, 0x0);
        digitalWrite(14, 0x1);
    }
    else {
        digitalWrite(15, 0x1);
        digitalWrite(16, 0x1);
        digitalWrite(14, 0x0);
    }

    odczytajPrzyciskZmiany();

    zmianaTemperaturyPrzyciski();

    roznicaCzasu[1] = aktualnyCzas - zapamietanyCzas[1];
    if (roznicaCzasu[1] >= 60000UL) {
        zapamietanyCzas[1] = aktualnyCzas;
        wyslijDaneNaSerwer();
    }

}

void wyswietlDaneNaLCD() {
    if(zmianaTrybu) {
        lcd.clear();
        zmianaTrybu = false;
    }
    switch(trybWyswietlacza) {
        case 0:
            lcd.setCursor(0,0);
            lcd.print("Temp: "+String(temperatura)+" "+char(223)+"C");
            lcd.setCursor(0,1);
            lcd.print("Zadana: "+String(temperaturaUstawiona)+" "+char(223)+"C");
            //Serial.println("\tDHT11");
            //Serial.println("Temperatura: "+String(temperatura)+" %C");
            //Serial.println("Wilgotność: "+String(wilgotnosc)+" %RH");
            break;
        case 1:
            lcd.setCursor(0,0);
            lcd.print("Wilg: "+String(wilgotnosc)+" %RH");
            break;
        default:
            lcd.setCursor(0,0);
            lcd.print("Wilg: "+String(wilgotnosc)+" %RH");
            lcd.setCursor(0,1);
            lcd.print("Temp: "+String(temperatura)+" %C");
            break;
    }
}

void zmianaTemperaturyPrzyciski() {
    int zczytanieStanuPlus = digitalRead(6);
    int zczytanieStanuMinus = digitalRead(5);
    if(zczytanieStanuPlus == 0x0 && ostatniStanPrzyciskPlus == 0x1) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona < 40) {
            staraTemperaturaUstawiona = temperaturaUstawiona;
            temperaturaUstawiona++;
        }
    }
    if(zczytanieStanuMinus == 0x0 && ostatniStanPrzyciskMinus == 0x1) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona > 15) {
            staraTemperaturaUstawiona = temperaturaUstawiona;
            temperaturaUstawiona--;
        }
    }
    wyswietlDaneNaLCD();
    ostatniStanPrzyciskPlus = zczytanieStanuPlus;
    ostatniStanPrzyciskMinus = zczytanieStanuMinus;
}

void odczytajPrzyciskZmiany() {
    int zczytanieStanu = digitalRead(3);
    if(zczytanieStanu == 0x0 && ostatniStanPrzyciskuZmiany == 0x1) {
        zmianaTrybu = true;
        if(trybWyswietlacza == 0) {
            trybWyswietlacza = 1;
        }
        else {
            trybWyswietlacza = 0;
        }
        wyswietlDaneNaLCD();
    }
    ostatniStanPrzyciskuZmiany = zczytanieStanu;
}

void wyslijDaneNaSerwer() {
    String queryString = String("/index.php?temperatura=") + String(temperatura) + String("&wilgotnosc=") + String(wilgotnosc) + String("&ustawiona=") + String(temperaturaUstawiona);
    //Serial.println(queryString);
    Serial.println("\nStarting connection to server...");
    if(client.connect(SERVER, HTTP_PORT)) {
        Serial.println("connected to server");
        // HTTP request
        client.println("GET "+ queryString + " HTTP/1.1");
        client.println("Host: " + String(SERVER));
        client.println("Connection: close");
        client.println(); // end HTTP header
        digitalWrite(17, 0x1);
        while(client.connected()) {
        }
        // the server's disconnected, stop the client:
        client.stop();
        digitalWrite(17, 0x0);
        Serial.println();
        Serial.println("disconnected");
        staraTemperaturaUstawiona = temperaturaUstawiona;
    }
    else { // if not connected:
        Serial.println("connection failed");
    }
}

int pobierzTemperature() {
    int liczba = temperaturaUstawiona;
    String queryString = String("/getValue.php");
    Serial.println("\nStarting connection to server...");
    if(client.connect(SERVER, HTTP_PORT)) {
        Serial.println("connected to server");
        client.println("GET "+ queryString + " HTTP/1.1");
        client.println("Host: " + String(SERVER));
        client.println("Connection: close");
        client.println();

        boolean czytanie = false;
        String str = "";
        digitalWrite(17, 0x1);
        while(client.connected()) {
            while(client.available()){
                char c = client.read();
                //Serial.print(c);
                if(czytanie) {
                    str = str + c;
                }
                if(c == '?') {
                    czytanie = true;
                }
                if(czytanie && c == ' ') {
                    czytanie == false;
                }
            }
        }
        liczba = str.toInt();
        if(liczba < 15 || liczba > 40) {
            liczba = temperaturaUstawiona;
        }
        //Serial.println();
        //Serial.println(str);
        //Serial.println(liczba);
        client.stop();
        digitalWrite(17, 0x0);
        Serial.println();
        Serial.println("disconnected");
    }
    else {
        Serial.println("connection failed");
    }

    return liczba;
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void connectWiFi() {
    digitalWrite(17, 0x1);
    // sprawdzenie WiFi shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 291 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 291 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      "WiFi shield not present"
# 291 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      ); &__c[0];}))
# 291 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      )));
        // don't continue:
        while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv != "1.1.0") {
        Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 297 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 297 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      "Please upgrade the firmware"
# 297 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      ); &__c[0];}))
# 297 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      )));
    }
    // próba łączenia z siecią WiFi
    while (status != WL_CONNECTED) {
        Serial.print((reinterpret_cast<const __FlashStringHelper *>(
# 301 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 301 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                    "Attempting to connect to SSID: "
# 301 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                    ); &__c[0];}))
# 301 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                    )));
        Serial.println(ssid);

        lcd.setCursor(0,0);
        lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 305 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 305 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 "Connecting to"
# 305 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 ); &__c[0];}))
# 305 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 )));
        lcd.setCursor(0,1);
        lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 307 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 307 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 "WiFi..."
# 307 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 ); &__c[0];}))
# 307 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 )));

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 2 seconds for connection:
        delay(2000);
    }

    // you're connected now, so print out the status:
    printWifiStatus();

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 320 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
             (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 320 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
             "Connect OK"
# 320 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
             ); &__c[0];}))
# 320 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
             )));
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());

    delay(2000);
    lcd.clear();

    digitalWrite(17, 0x0);
}
# 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\test.ino"

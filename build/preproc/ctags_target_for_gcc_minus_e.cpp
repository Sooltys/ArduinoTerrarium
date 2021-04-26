# 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
// biblioteka Arduino pozwalająca na komunikację z urządzeniami I2C na porcie SDA i SCL
# 3 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
// biblioteki do obsługi wyświetlacza LCD z konwerterem I2C
# 5 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
// biblioteki do obsługi czujnika temperatury
# 7 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
# 8 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
// biblioteki poptrzebne do osługi nakładi WiFi komunikującego się z mikrokontrolerem za pomocą interfejsu SPI
# 10 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
# 11 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2
// biblioteka funkcji watchdog 
# 13 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 2


# 14 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
// definiujemy rodzaj czujnika temperatury oraz do którego portu jest podłączony


// definiujemy porty







// definiujemy wartości temperatur



// definiujemy czasy wykonywania funkcji







// wypisujemy zmienne oraz ich wartości początkowe
int ostatniStanPrzyciskPlus = 0x1;
int ostatniStanPrzyciskMinus = 0x1;
int temperaturaUstawiona = 19;
boolean zmianaTemperatury = false;
float wilgotnosc;
float temperatura;
int trybWyswietlacza = 0;
boolean zmianaTrybu = false;
int ostatniStanPrzyciskuZmiany = 0x1;

// zmienne do odliczania millis()
unsigned long aktualnyCzas = 100000;
unsigned long zapamietanyCzas[] = {0, 0, 0};
unsigned long roznicaCzasu[] = {0, 0, 0};

// zmienne logiky rozmytej
unsigned long czasPrzyrostu = 0;
float przyrost = 0.0;
float ostatniaTemperatura = 0.0;
unsigned long ponowneWprowadzenie = 0.0;

// zmienne sieci WiFi
//char ssid[] = "FunBox2-EF66";       // SSID sieci WiFi
//char pass[] = "NIEMAHASLA";         // haslo sieci WiFi
char ssid[] = "iPhone (Jakub)"; // SSID sieci WiFi
char pass[] = "qwerty123"; // haslo sieci WiFi
//int keyIndex = 0;                 // dla sieci zabezpieczonej WEP
int status = WL_IDLE_STATUS;

// zmienne łączenia z serwerem
WiFiClient client;
char SERVER[] = "www.pj41491.zut.edu.pl";
int HTTP_PORT = 80;

// inicjalizacja czujnika temperatury i wilgotności
DHT dht(2, 11 /**< DHT TYPE 11 */);

// inicjalizacja wyświetlacza 
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
    Serial.begin(9600); // szybkość komunikacji szeregowej

    // definicja rodzaju pinów mikrokontrolera
    pinMode(3, 0x2);
    pinMode(6, 0x2);
    pinMode(5, 0x2);
    pinMode(15, 0x1);
    pinMode(16, 0x1);
    pinMode(14, 0x1);
    pinMode(17, 0x1);
    // przypisanie poczatkowych stanów dla pinów
    digitalWrite(15, 0x1);
    digitalWrite(16, 0x1);
    digitalWrite(14, 0x0);
    digitalWrite(17, 0x0);

    dht.begin(); // start czujnika temperatury

    lcd.begin(); // start wyświetlacza z włączeniem podświetlenia
    lcd.backlight();
    lcd.clear();

    connectWiFi(); // połączenie z siecią WiFi

    wdt_enable(
# 103 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
              9
# 103 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                     ); // uruchomienie watchdog'a
}

void loop() {
    
# 107 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
   __asm__ __volatile__ ("wdr")
# 107 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
              ; // reset licznika watchdog'a

    roznicaCzasu[2] = aktualnyCzas - zapamietanyCzas[2];
    if (roznicaCzasu[2] >= 2000UL) {
        zapamietanyCzas[2] = aktualnyCzas;
        wilgotnosc = dht.readHumidity();
        temperatura = dht.readTemperature();
        wyswietlDaneNaLCD();
    }

    roznicaCzasu[0] = aktualnyCzas - zapamietanyCzas[0];
    if (roznicaCzasu[0] >= 20000UL) {
        zapamietanyCzas[0] = aktualnyCzas;
        if(zmianaTemperatury == false && status == WL_CONNECTED) {
            temperaturaUstawiona = pobierzTemperature();
        }
    }

    // reset licznika watchdog'a
    
# 126 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
   __asm__ __volatile__ ("wdr")
# 126 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
              ;

    odczytajPrzyciskZmiany(); // obsługa przycisku zmiany wyświetlania

    zmianaTemperaturyPrzyciski(); // obsługa przyciskó zmiany temperatury

    roznicaCzasu[1] = aktualnyCzas - zapamietanyCzas[1];
    if (roznicaCzasu[1] >= 60000UL) {
        zapamietanyCzas[1] = aktualnyCzas;
        if(status == WL_CONNECTED) {
            wyslijDaneNaSerwer();
        }
    }

    // wykorzystanie millis() zamiast delay() aby nie zatrzymywać całego mikrokontrolera
    aktualnyCzas = millis();

    //sterowanieTemperatury(); // algorytm sterowania temperatura
    sterowanieTemperaturyRozmyte();
}

void sterowanieTemperatury() {
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
}

void sterowanieTemperaturyRozmyte() {
    // wyliczanie przyrostu co 15 sekund
    unsigned long roznicaCzasuPrzyrostu = aktualnyCzas - czasPrzyrostu;
    if(roznicaCzasuPrzyrostu >= 15000UL) {
        czasPrzyrostu = aktualnyCzas;
        przyrost = temperatura - ostatniaTemperatura;
        ostatniaTemperatura = temperatura;
    }

    int fuzzyTemperatura; //niska-0, lekko niska-1, prawie osiagnieta-2, osiagnieta-3, wysoka-4
    int fuzzyPrzyrost; // maly-0, duzy-1
    if(przyrost >= 0.15) {
        fuzzyPrzyrost = 1;
    }else {
        fuzzyPrzyrost = 0;
    }
    if(((temperaturaUstawiona*1.0) - temperatura) >= 3) {
        fuzzyTemperatura = 0;
    }else if(((temperaturaUstawiona*1.0) - temperatura) >= 0.9) {
        fuzzyTemperatura = 1;
    }else if(((temperaturaUstawiona*1.0) - temperatura) >= 0.2) {
        fuzzyTemperatura = 2;
    }else if(((temperaturaUstawiona*1.0) - temperatura) >= -0.2) {
        fuzzyTemperatura = 3;
    }else {
        fuzzyTemperatura = 4;
    }

    boolean grzanie = false;
    unsigned long czasGrzania = 0UL;
    if(fuzzyTemperatura == 0 && fuzzyPrzyrost == 0) {
        grzanie = true;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 1 && fuzzyPrzyrost == 0) {
        grzanie = true;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 2 && fuzzyPrzyrost == 0) {
        grzanie = true;
        czasGrzania = 20000UL;
    }else if(fuzzyTemperatura == 3 && fuzzyPrzyrost == 0) {
        grzanie = false;
        czasGrzania = 15000UL;
    }else if(fuzzyTemperatura == 4 && fuzzyPrzyrost == 0) {
        grzanie = false;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 0 && fuzzyPrzyrost == 1) {
        grzanie = true;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 1 && fuzzyPrzyrost == 1) {
        grzanie = true;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 2 && fuzzyPrzyrost == 1) {
        grzanie = false;
        czasGrzania = 15000UL;
    }else if(fuzzyTemperatura == 3 && fuzzyPrzyrost == 1) {
        grzanie = false;
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 4 && fuzzyPrzyrost == 1) {
        grzanie = false;
        czasGrzania = 40000UL;
    }
    Serial.println(String(((temperaturaUstawiona*1.0) - temperatura)) +" = "+ String(fuzzyTemperatura) +" -- "+ String(fuzzyPrzyrost)+" = "+ String(przyrost));
    Serial.println("Ponowne: " + String(ponowneWprowadzenie) + "czas: " + String(aktualnyCzas));
    if(aktualnyCzas > ponowneWprowadzenie) {
        if(grzanie == true) {
            digitalWrite(15, 0x0);
            digitalWrite(16, 0x0);
            digitalWrite(14, 0x1);
        }
        else {
            digitalWrite(15, 0x1);
            digitalWrite(16, 0x1);
            digitalWrite(14, 0x0);
        }
        ponowneWprowadzenie = aktualnyCzas + czasGrzania;
    }
}

void wyswietlDaneNaLCD() {
    if(zmianaTrybu) {
        lcd.clear();
        zmianaTrybu = false;
    }
    switch(trybWyswietlacza) {
        case 0:
            lcd.backlight();
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
        case 2:
            lcd.setCursor(0,0);
            lcd.print("Status WiFi");
            lcd.setCursor(0,1);
            if(status == WL_CONNECTED)
                lcd.print("OK");
            else
                lcd.print("ERROR");
            break;
        case 3:
            lcd.noBacklight();
            lcd.setCursor(0,0);
            lcd.print("Temp: "+String(temperatura)+" "+char(223)+"C");
            lcd.setCursor(0,1);
            lcd.print("Zadana: "+String(temperaturaUstawiona)+" "+char(223)+"C");
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
            zmianaTemperatury = true;
            temperaturaUstawiona++;
        }
    }
    if(zczytanieStanuMinus == 0x0 && ostatniStanPrzyciskMinus == 0x1) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona > 15) {
            zmianaTemperatury = true;
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
        else if(trybWyswietlacza == 1) {
            trybWyswietlacza = 2;
        }
        else if(trybWyswietlacza == 2) {
            trybWyswietlacza = 3;
        }
        else {
            trybWyswietlacza = 0;
        }
        wyswietlDaneNaLCD();
    }
    ostatniStanPrzyciskuZmiany = zczytanieStanu;
}

void wyslijDaneNaSerwer() {
    digitalWrite(17, 0x1);
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
        while(client.connected()) { }
        // the server's disconnected, stop the client:
        client.stop();
        Serial.println();
        Serial.println("disconnected");
        zmianaTemperatury = false;
    }
    else { // if not connected:
        Serial.println("connection failed");
    }
    digitalWrite(17, 0x0);
}

int pobierzTemperature() {
    digitalWrite(17, 0x1);
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
        Serial.println();
        Serial.println("disconnected");
    }
    else {
        Serial.println("connection failed");
    }
    digitalWrite(17, 0x0);

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
    int proba = 0;
    digitalWrite(17, 0x1);
    // sprawdzenie WiFi shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 419 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 419 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      "WiFi shield not present"
# 419 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      ); &__c[0];}))
# 419 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      )));
        // zawieszenie programu
        while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv != "1.1.0") {
        Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 425 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 425 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      "Please upgrade the firmware"
# 425 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                      ); &__c[0];}))
# 425 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                      )));
    }
    // próba łączenia z siecią WiFi
    while (status != WL_CONNECTED && proba == 0) {
        proba = 1;
        Serial.print((reinterpret_cast<const __FlashStringHelper *>(
# 430 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                    (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 430 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                    "Attempting to connect to SSID: "
# 430 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                    ); &__c[0];}))
# 430 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                    )));
        Serial.println(ssid);

        lcd.setCursor(0,0);
        lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 434 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 434 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 "Connecting to"
# 434 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 ); &__c[0];}))
# 434 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 )));
        lcd.setCursor(0,1);
        lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 436 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 436 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 "WiFi..."
# 436 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 ); &__c[0];}))
# 436 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 )));

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 2 seconds for connection:
        delay(2000);
    }

    if(status == WL_CONNECTED) {
        printWifiStatus();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print((reinterpret_cast<const __FlashStringHelper *>(
# 448 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 448 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 "Connect OK"
# 448 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino" 3
                 ); &__c[0];}))
# 448 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
                 )));
        lcd.setCursor(0,1);
        lcd.print(WiFi.localIP());
        delay(2000);
        lcd.clear();
    }

    digitalWrite(17, 0x0);
}
# 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\test.ino"

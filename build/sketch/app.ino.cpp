#include <Arduino.h>
#line 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <DHT.h>
#include <DHT_U.h>

#include <SPI.h>
#include <WiFi.h>

#include <avr/wdt.h>

// czujjnik temperatury i wilgoci
#define DHTPIN 2
#define DHTTYPE DHT11

#define uploadLED 17
#define przyciskZmianyTrybuLCD 3
#define przyciskPlus 6
#define przyciskMinus 5
#define grzalka 15
#define grzalkaMata 16
#define grzalkaLED 14
#define minimalnaTemperatura 15
#define maksymalnaTemperatura 40
#define nominalnaTemperatura 30

int ostatniStanPrzyciskPlus = HIGH;
int ostatniStanPrzyciskMinus = HIGH;
int temperaturaUstawiona = nominalnaTemperatura;
int staraTemperaturaUstawiona = temperaturaUstawiona;
float wilgotnosc;
float temperatura;
int trybWyswietlacza = 0;
boolean zmianaTrybu = false;
int ostatniStanPrzyciskuZmiany = HIGH;

// inicjalizacja czujnika temp i wilg
DHT dht(DHTPIN, DHTTYPE);

// inicjalizacja wyświetlacza 
LiquidCrystal_I2C lcd(0x27, 16, 2);

// zmienne do odliczania millis()
unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzas[] = {-10000, -60000, -5000};
unsigned long roznicaCzasu[] = {0, 0, 0};

// zmienne sieci WiFi
char ssid[] = "FunBox2-EF66";       // SSID sieci WiFi
char pass[] = "NIEMAHASLA";         // haslo sieci WiFi
//int keyIndex = 0;                   // dla sieci zabezpieczonej WEP
int status = WL_IDLE_STATUS;

// zmienne łączenia z serwerem
WiFiClient client;
char SERVER[] = "www.pj41491.zut.edu.pl";
int HTTP_PORT = 80;

#line 59 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void setup();
#line 85 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void loop();
#line 133 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void wyswietlDaneNaLCD();
#line 161 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void zmianaTemperaturyPrzyciski();
#line 181 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void odczytajPrzyciskZmiany();
#line 196 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void wyslijDaneNaSerwer();
#line 222 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
int pobierzTemperature();
#line 270 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void printWifiStatus();
#line 287 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void connectWiFi();
#line 59 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\app.ino"
void setup() {
    Serial.begin(9600);

    pinMode(przyciskZmianyTrybuLCD, INPUT_PULLUP);
    pinMode(przyciskPlus, INPUT_PULLUP);
    pinMode(przyciskMinus, INPUT_PULLUP);
    pinMode(grzalka, OUTPUT);
    pinMode(grzalkaLED, OUTPUT);
    pinMode(grzalkaMata, OUTPUT);
    digitalWrite(grzalka, HIGH);
    digitalWrite(grzalkaLED, LOW);
    digitalWrite(grzalkaMata, HIGH);
    pinMode(uploadLED, OUTPUT);
  
    dht.begin();

    lcd.begin();
    lcd.backlight();
    lcd.clear();

    connectWiFi();

    // ustawienie watchdog'a
    wdt_enable(WDTO_8S);
}

void loop() {
    // reset licznika watchdog'a
    wdt_reset();
  
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
    wdt_reset();

    if(temperaturaUstawiona > temperatura) {
        digitalWrite(grzalka, LOW);
        digitalWrite(grzalkaMata, LOW);
        digitalWrite(grzalkaLED, HIGH);
    }
    else {
        digitalWrite(grzalka, HIGH);
        digitalWrite(grzalkaMata, HIGH);
        digitalWrite(grzalkaLED, LOW);
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
    int zczytanieStanuPlus = digitalRead(przyciskPlus);
    int zczytanieStanuMinus = digitalRead(przyciskMinus);
    if(zczytanieStanuPlus == LOW && ostatniStanPrzyciskPlus == HIGH) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona < maksymalnaTemperatura) {
            staraTemperaturaUstawiona = temperaturaUstawiona;
            temperaturaUstawiona++;
        }
    }
    if(zczytanieStanuMinus == LOW && ostatniStanPrzyciskMinus == HIGH) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona > minimalnaTemperatura) {
            staraTemperaturaUstawiona = temperaturaUstawiona;
            temperaturaUstawiona--;
        }
    }
    wyswietlDaneNaLCD();
    ostatniStanPrzyciskPlus = zczytanieStanuPlus;
    ostatniStanPrzyciskMinus = zczytanieStanuMinus;
}

void odczytajPrzyciskZmiany() {
    int zczytanieStanu = digitalRead(przyciskZmianyTrybuLCD);
    if(zczytanieStanu == LOW && ostatniStanPrzyciskuZmiany == HIGH) {
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
        digitalWrite(uploadLED, HIGH);
        while(client.connected()) {
        }
        // the server's disconnected, stop the client:
        client.stop();
        digitalWrite(uploadLED, LOW);
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
        digitalWrite(uploadLED, HIGH);
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
        if(liczba < minimalnaTemperatura || liczba > maksymalnaTemperatura) {
            liczba = temperaturaUstawiona;
        }
        //Serial.println();
        //Serial.println(str);
        //Serial.println(liczba);
        client.stop();
        digitalWrite(uploadLED, LOW);
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
    digitalWrite(uploadLED, HIGH);
    // sprawdzenie WiFi shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println(F("WiFi shield not present"));
        // don't continue:
        while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv != "1.1.0") {
        Serial.println(F("Please upgrade the firmware"));
    }
    // próba łączenia z siecią WiFi
    while (status != WL_CONNECTED) {
        Serial.print(F("Attempting to connect to SSID: "));
        Serial.println(ssid);
    
        lcd.setCursor(0,0);
        lcd.print(F("Connecting to"));
        lcd.setCursor(0,1);
        lcd.print(F("WiFi..."));
    
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 2 seconds for connection:
        delay(2000);
    }

    // you're connected now, so print out the status:
    printWifiStatus();

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Connect OK"));
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
  
    delay(2000);
    lcd.clear();
    
    digitalWrite(uploadLED, LOW);
}
#line 1 "c:\\Users\\Kuba\\Desktop\\Praca dyplomowa\\ArduinoTerrarium\\test.ino"


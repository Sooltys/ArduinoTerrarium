// biblioteka Arduino pozwalająca na komunikację z urządzeniami I2C na porcie SDA i SCL
#include <Wire.h>
// biblioteki do obsługi wyświetlacza LCD z konwerterem I2C
#include <LiquidCrystal_I2C.h>
// biblioteki do obsługi czujnika temperatury
#include <DHT.h>
#include <DHT_U.h>
// biblioteki poptrzebne do osługi nakładi WiFi komunikującego się z mikrokontrolerem za pomocą interfejsu SPI
#include <SPI.h>
#include <WiFi.h>
// biblioteka funkcji watchdog 
#include <avr/wdt.h>

// definiujemy rodzaj czujnika temperatury oraz do którego portu jest podłączony
#define DHTPIN 2
#define DHTTYPE DHT11
// definiujemy porty
#define komunikacjaLED 17
#define przyciskZmianyTrybuLCD 3
#define przyciskPlus 6
#define przyciskMinus 5
#define grzalka 15
#define grzalkaMata 16
#define grzalkaLED 14
// definiujemy wartości temperatur
#define minimalnaTemperatura 15
#define maksymalnaTemperatura 40
#define nominalnaTemperatura 19
// definiujemy czasy wykonywania funkcji
#define nrCzasCzytanie 2
#define czasCzytanieCzujnik 2000UL
#define nrCzasPobieranie 0
#define czasPobieranieTemp 20000UL
#define nrCzasWysylanie 1
#define czasWysylanieTemp 60000UL

// wypisujemy zmienne oraz ich wartości początkowe
int ostatniStanPrzyciskPlus = HIGH;
int ostatniStanPrzyciskMinus = HIGH;
int temperaturaUstawiona = nominalnaTemperatura;
boolean zmianaTemperatury = false;
float wilgotnosc;
float temperatura;
int trybWyswietlacza = 0;
boolean zmianaTrybu = false;
int ostatniStanPrzyciskuZmiany = HIGH;

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
char ssid[] = "FunBox2-EF66";       // SSID sieci WiFi
char pass[] = "NIEMAHASLA";         // haslo sieci WiFi
//char ssid[] = "iPhone (Jakub)";       // SSID sieci WiFi
//char pass[] = "qwerty123";          // haslo sieci WiFi
//int keyIndex = 0;                 // dla sieci zabezpieczonej WEP
int status = WL_IDLE_STATUS;

// zmienne łączenia z serwerem
WiFiClient client; 
char SERVER[] = "www.pj41491.zut.edu.pl";
int HTTP_PORT = 80;

// inicjalizacja czujnika temperatury i wilgotności
DHT dht(DHTPIN, DHTTYPE); 

// inicjalizacja wyświetlacza 
LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
    Serial.begin(9600); // szybkość komunikacji szeregowej

    // definicja rodzaju pinów mikrokontrolera
    pinMode(przyciskZmianyTrybuLCD, INPUT_PULLUP);
    pinMode(przyciskPlus, INPUT_PULLUP);
    pinMode(przyciskMinus, INPUT_PULLUP);
    pinMode(grzalka, OUTPUT);
    pinMode(grzalkaMata, OUTPUT);
    pinMode(grzalkaLED, OUTPUT);
    pinMode(komunikacjaLED, OUTPUT);
    // przypisanie poczatkowych stanów dla pinów
    digitalWrite(grzalka, HIGH);
    digitalWrite(grzalkaMata, HIGH);
    digitalWrite(grzalkaLED, LOW);
    digitalWrite(komunikacjaLED, LOW);
  
    dht.begin(); // start czujnika temperatury

    lcd.begin(); // start wyświetlacza z włączeniem podświetlenia
    lcd.backlight();
    lcd.clear();

    connectWiFi(); // połączenie z siecią WiFi

    wdt_enable(WDTO_8S); // uruchomienie watchdog'a
}

void loop() {
    wdt_reset(); // reset licznika watchdog'a

    roznicaCzasu[nrCzasCzytanie] = aktualnyCzas - zapamietanyCzas[nrCzasCzytanie];
    if (roznicaCzasu[nrCzasCzytanie] >= czasCzytanieCzujnik) {
        zapamietanyCzas[nrCzasCzytanie] = aktualnyCzas;
        wilgotnosc = dht.readHumidity();
        temperatura = dht.readTemperature();
        wyswietlDaneNaLCD();
    }

    roznicaCzasu[nrCzasPobieranie] = aktualnyCzas - zapamietanyCzas[nrCzasPobieranie];
    if (roznicaCzasu[nrCzasPobieranie] >= czasPobieranieTemp) {
        zapamietanyCzas[nrCzasPobieranie] = aktualnyCzas;
        if(zmianaTemperatury == false && status == WL_CONNECTED) {
            temperaturaUstawiona = pobierzTemperature();
        }
    }

    // reset licznika watchdog'a
    wdt_reset();

    odczytajPrzyciskZmiany(); // obsługa przycisku zmiany wyświetlania

    zmianaTemperaturyPrzyciski(); // obsługa przyciskó zmiany temperatury
  
    roznicaCzasu[nrCzasWysylanie] = aktualnyCzas - zapamietanyCzas[nrCzasWysylanie];
    if (roznicaCzasu[nrCzasWysylanie] >= czasWysylanieTemp) {
        zapamietanyCzas[nrCzasWysylanie] = aktualnyCzas;
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
        digitalWrite(grzalka, LOW);
        digitalWrite(grzalkaMata, LOW);
        digitalWrite(grzalkaLED, HIGH);
    }
    else {
        digitalWrite(grzalka, HIGH);
        digitalWrite(grzalkaMata, HIGH);
        digitalWrite(grzalkaLED, LOW);
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
    if(fuzzyTemperatura == 0 &&  fuzzyPrzyrost == 0) {
        grzanie = true; 
        czasGrzania = 40000UL;
    }else if(fuzzyTemperatura == 1 &&  fuzzyPrzyrost == 0) {
        grzanie = true; 
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 2 &&  fuzzyPrzyrost == 0) {
        grzanie = true; 
        czasGrzania = 20000UL;
    }else if(fuzzyTemperatura == 3 &&  fuzzyPrzyrost == 0) {
        grzanie = false; 
        czasGrzania = 15000UL;
    }else if(fuzzyTemperatura == 4 &&  fuzzyPrzyrost == 0) {
        grzanie = false; 
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 0 &&  fuzzyPrzyrost == 1) {
        grzanie = true; 
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 1 &&  fuzzyPrzyrost == 1) {
        grzanie = true; 
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 2 &&  fuzzyPrzyrost == 1) {
        grzanie = false; 
        czasGrzania = 15000UL;
    }else if(fuzzyTemperatura == 3 &&  fuzzyPrzyrost == 1) {
        grzanie = false; 
        czasGrzania = 30000UL;
    }else if(fuzzyTemperatura == 4 &&  fuzzyPrzyrost == 1) {
        grzanie = false; 
        czasGrzania = 40000UL;
    }
    Serial.println(String(((temperaturaUstawiona*1.0) - temperatura)) +" = "+ String(fuzzyTemperatura) +" -- "+ String(fuzzyPrzyrost)+" = "+ String(przyrost));
    Serial.println("Ponowne: " + String(ponowneWprowadzenie) + "czas: " + String(aktualnyCzas));
    if(aktualnyCzas > ponowneWprowadzenie) {
        if(grzanie == true) {
            digitalWrite(grzalka, LOW);
            digitalWrite(grzalkaMata, LOW);
            digitalWrite(grzalkaLED, HIGH);
        }
        else {
            digitalWrite(grzalka, HIGH);
            digitalWrite(grzalkaMata, HIGH);
            digitalWrite(grzalkaLED, LOW);
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
    int zczytanieStanuPlus = digitalRead(przyciskPlus);
    int zczytanieStanuMinus = digitalRead(przyciskMinus);
    if(zczytanieStanuPlus == LOW && ostatniStanPrzyciskPlus == HIGH) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona < maksymalnaTemperatura) {
            zmianaTemperatury = true;
            temperaturaUstawiona++;
        }
    }
    if(zczytanieStanuMinus == LOW && ostatniStanPrzyciskMinus == HIGH) {
        if(trybWyswietlacza == 0 && temperaturaUstawiona > minimalnaTemperatura) {
            zmianaTemperatury = true;
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
    digitalWrite(komunikacjaLED, HIGH);
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
    digitalWrite(komunikacjaLED, LOW);
}

int pobierzTemperature() {
    digitalWrite(komunikacjaLED, HIGH);
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
        if(liczba < minimalnaTemperatura || liczba > maksymalnaTemperatura) {
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
    digitalWrite(komunikacjaLED, LOW);

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
    digitalWrite(komunikacjaLED, HIGH);
    // sprawdzenie WiFi shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println(F("WiFi shield not present"));
        // zawieszenie programu
        while (true);
    }
    String fv = WiFi.firmwareVersion();
    if (fv != "1.1.0") {
        Serial.println(F("Please upgrade the firmware"));
    }
    // próba łączenia z siecią WiFi
    while (status != WL_CONNECTED && proba == 0) {
        proba = 1;
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

    if(status == WL_CONNECTED) {
        printWifiStatus();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("Connect OK"));
        lcd.setCursor(0,1);
        lcd.print(WiFi.localIP());
        delay(2000);
        lcd.clear();
    }
    
    digitalWrite(komunikacjaLED, LOW);
}
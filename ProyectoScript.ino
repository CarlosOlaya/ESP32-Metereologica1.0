#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>  // Añade esta línea al inicio

// librerias de conexion
#include <WiFi.h>
#include <HTTPClient.h>
 
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Usa 0x3F si tu LCD usa esa dirección

// Reemplaza por tu red chuzo
const char* ssid = "DIVERSO";
const char* password = "Diverso#2024";

//const char* ssid = "moto84";
//const char* password = "5267812g";

//const char* ssid = "yosoycecar";
// const char* password = "yosoycecar";

//const char* ssid = "HOGAR OLAYA";
//const char* password = "Ashtom428";

// API Key de escritura de ThingSpeak
const char* apiKey = "MLEXB9YCED9UGH4C";
const char* server = "http://api.thingspeak.com/update";

#define DHTPIN  13

// Configuración I2C para ESP32
#define SDA_PIN 21
#define SCL_PIN 22

#define BUZZER 32
#define LED_VERDE 33
#define LED_AMARILLO 25
#define LED_ROJO 26
  
int pinSensor = 12;
int pinLed = 14;

DHT dht(DHTPIN, DHT22); 
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
// Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK); // Configuración SPI

Adafruit_BMP280 bmp;
float presionPrevia = 1010;
unsigned long ultimaLectura = 0;
const unsigned long intervalo3Horas = 10000; // 3 horas en milisegundos (para pruebas usa 60000)

void alertaOlaDeCalor() {
  Serial.println("⚠️ Alerta: Ola de Calor");
  digitalWrite(LED_ROJO, HIGH); 
  tone(BUZZER, 1000);
  delay(2000);
}

void alertaAltaHumedad() {
  Serial.println("⚠️ Alerta: Alta Humedad / Posible Lluvia");
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_AMARILLO, HIGH);
    delay(500);
    digitalWrite(LED_AMARILLO, LOW);
    delay(500);
  }
}

void alertaTormenta() {
  Serial.println("⚠️ Alerta: Tormenta en Camino");
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
    tone(BUZZER, 1000);
    delay(500);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_AMARILLO, LOW);
    delay(500);
  }
}

void condicionesEstables() {
  Serial.println("✅ Condiciones normales");
  digitalWrite(LED_VERDE, HIGH);
  delay(2000);
}

void apagarAlertas() {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(pinLed,LOW);  
  noTone(BUZZER);      // Apaga el buzzer
}

void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i=0; i < lcdColumns; i++) {
    message = " " + message; 
  } 
  message = message + " "; 
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// Variables para el control del LED de vibración
unsigned long previousVibMillis = 0;
const long vibInterval = 1000; // Intervalo de parpadeo en ms

void setup() {
  
  Serial.begin(115200);
  while(!Serial); // Espera solo si es USB nativo
  
  // Inicializa la LCD
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  String messageToConectando = "E. Meteorologica";
  String messageToWifi = "Conectando al WiFi";
  String messageToWifiON = "Conectado a WiFi";

  lcd.print(messageToConectando);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
     lcd.setCursor(0,1);
     scrollText(1, messageToWifi, 250, 16);
     Serial.println(messageToWifi);
     delay(1000);
     Serial.print(".");
  }
  lcd.setCursor(0,1);
  scrollText(1, messageToWifiON, 250, 16);
  
  Serial.println("\nConectado a WiFi.");
  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(pinSensor,INPUT);
  pinMode(pinLed,OUTPUT);
  
  dht.begin();
  
  Serial.println("\nIniciando BMP280...");
  
  bool status = bmp.begin(0x76); // Primero prueba 0x76
  if(!status) {
    status = bmp.begin(0x77);   // Luego prueba 0x77
    Serial.println("Probando dirección 0x77...");
  }
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
  Serial.println("Sensores inicializados.");
  delay(100);
}

void loop() {
  unsigned long currentMillis = millis();
  int vibracion = 0; 

  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();
  float presion = bmp.readPressure() / 100.0F;  // hPa en aplicacion
  float altitud = bmp.readAltitude(1010);
  float temperaturabmp280 = bmp.readTemperature();
  /*
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("Error obteniendo datos del DHT22");
    return;
  }
  */
  apagarAlertas();
  
  bool alertaGenerada = false;
  
  Serial.print("Presion: ");
  Serial.print(presion);
  Serial.print(" hPa\t");
  Serial.print("Altitud: ");
  Serial.print(altitud);
  Serial.print(" m\t");
  Serial.print("Temperaturabmp280: ");
  Serial.print(temperaturabmp280);
  Serial.print(" °C\t");
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.print(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(temperatura);  
  Serial.println(" °C");
  Serial.print(pinSensor);
  
  static unsigned long previousLCDMillis = 0;
  if (millis() - previousLCDMillis >= 1000) {
    previousLCDMillis = millis();
    
    lcd.setCursor(0, 0);  // Columna 0, Fila 0
    lcd.print("T:");
    lcd.print(temperatura, 1); // 1 decimal
    lcd.print("C ");
    lcd.print("A:");
    lcd.print(altitud, 2); // 1 decimal
    lcd.print("m");
    
    lcd.setCursor(0, 1);  // Columna 0, Fila 1
    lcd.print("P:");
    lcd.print(presion, 2); // 1 decimal
    lcd.print("h ");
    lcd.print("H:");
    lcd.print(humedad, 0); // Sin decimales
    lcd.print("%");


  }
 // Control del LED de vibraciónn
  if (digitalRead(pinSensor)) {
    if (currentMillis - previousVibMillis >= vibInterval) {
      previousVibMillis = currentMillis;
      Serial.println("✅ Condiciones normales");
      digitalWrite(pinLed, HIGH);
      vibracion = 1;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://api.thingspeak.com/update?api_key=") + apiKey +
                 "&field1=" + String(temperatura) +
                 "&field2=" + String(humedad) +
                 "&field3=" + String(presion) +
                 "&field4=" + String(altitud) +
                 "&field5=" + String(vibracion);// cabiar por un valor 0 1 del sensor
    http.begin(url);
    int httpCode = http.GET();
      if (httpCode > 0) {
          Serial.println("Datos enviados a ThingSpeak correctamente.");
      } else {
          Serial.println("Error al enviar datos.");
      }
      http.end();
    } else {
      Serial.println("WiFi no conectado.");
    }
    

  float diferenciaPresion = presionPrevia - presion;
 
  
  if (temperatura > 35 && humedad < 60) { //40% real
    alertaOlaDeCalor();
    alertaGenerada = true;
  } 
  else if (humedad > 85 && diferenciaPresion > 5) {
    alertaAltaHumedad();
    alertaGenerada = true;
  }
  else if (presion < 1005 && diferenciaPresion > 10) {
    alertaTormenta();
    alertaGenerada = true;
  }
  if (!alertaGenerada) {
    condicionesEstables();
  }
  if (millis() - ultimaLectura >= intervalo3Horas) {
    presionPrevia = presion;
    ultimaLectura = millis();
  }
 



}

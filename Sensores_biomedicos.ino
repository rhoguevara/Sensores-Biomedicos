//Proyecto de investigacion sobre sensores biomédicos.
//Código de programación Sensor de Temperatura:

#include "ThingsBoard.h"
#include <ESP8266WiFi.h>
#include <Adafruit_MLX90614.h>
#include <Wire.h>
#define WIFI_AP "Proyecto Final - A3"
#define WIFI_PASSWORD "ProyectoFinal-A3"
#define TOKEN "h3r3irIPFYtqds1lXLAv"
#define THINGSBOARD_SERVER "demo.thingsboard.io"
#define SERIAL_DEBUG_BAUD 115200
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
int bd[900];
int count;
Adafruit_MLX90614 termometroIR = Adafruit_MLX90614();
void setup() {
 // initialize serial for debugging
 Serial.begin(SERIAL_DEBUG_BAUD);
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 InitWiFi();
 delay(500);
 termometroIR.begin();
 reconnect();
}
void loop() {
 delay(1000);
 if (WiFi.status() != WL_CONNECTED) {
 Serial.println("Se ha perdido la conexión!");
 reconnect();
 }
 if (count == 900){
 count = 0;
 // Enviar datos a la DB_MYSQL;
 }
 
 if (!tb.connected()) {
 // Connect to the ThingsBoard
 Serial.print("Conectando a: ");
 Serial.print(THINGSBOARD_SERVER);
 Serial.print(" con token ");
 Serial.println(TOKEN);
 if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
 Serial.println("Error en la conexión");
 return;
 }
 }
 
 float tempAmbiente = termometroIR.readAmbientTempC();
 float tempObjeto = termometroIR.readObjectTempC();
113
 // Mostrar información
 Serial.print("Temp. ambiente: ");
 Serial.print(tempAmbiente);
 Serial.println("ºC");
 
 Serial.print("Temp. objeto: ");
 Serial.print(tempObjeto);
 Serial.println("ºC");
 if (tempObjeto <= 45 && tempObjeto >= 30){
 tb.sendTelemetryInt("Temperatura", tempObjeto);
 tb.loop();
 Serial.println("Enviando datos...");
 }
}
// Functions
void InitWiFi()
{
 Serial.println("Conectando al AP ...");
 // attempt to connect to WiFi network
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.println("Conectado al AP!");
}
void reconnect() {
 // Loop until we're reconnected
 status = WiFi.status();
 if ( status != WL_CONNECTED) {
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.println("Conectado al AP");
 }
}

//Código de programación Sensor de Oximetría:

#include "ThingsBoard.h"
#include <ESP8266WiFi.h>
#include "MAX30105.h" // MAX3010X libreria
#include <Wire.h>
#include "heartRate.h" 
#define WIFI_AP "Proyecto Final - A3"
#define WIFI_PASSWORD "ProyectoFinal-A3"
#define TOKEN "8OSMIqUJjY5Pw3aPWQZB"
#define THINGSBOARD_SERVER "demo.thingsboard.io"
#define SERIAL_DEBUG_BAUD 115200
MAX30105 particleSensor;
const byte RATE_SIZE = 4; //incremento para obtener más promedios. 4 
es bueno.
byte rates[RATE_SIZE]; //matriz de frecuencias cardiacas 
byte rateSpot = 0;
long ultimolatido = 0; //Hora a la que ocurrió el último latido
float latidosporminutos;
int mejorpromedio;
double oxi;
#define USEFIFO
// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;
void setup() {
 // initialize serial for debugging
 Serial.begin(SERIAL_DEBUG_BAUD);
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 InitWiFi();
 delay(500);
 // Initialize sensor
 if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Utilice el puerto 
I2C predeterminado, velocidad de 400 kHz
 {
 Serial.println("MAX30102 was not found. Please check 
wiring/power/solder jumper at MH-ET LIVE MAX30102 board.");
 while (1);
 }
 byte ledBrightness = 70; //Opciones: 0 = Apagado a 255 = 50mA
 byte sampleAverage = 4; //Opciones: 1, 2, 4, 8, 16, 32
 byte ledMode = 2; //Opciones: 1 = Solo rojo, 2 = Rojo + IR, 3 = Rojo 
+ IR + Verde
 // Opciones: 1 = solo IR, 2 = Rojo + IR en placa MH-ET LIVE MAX30102
 int sampleRate = 200; //Opciones: 50, 100, 200, 400, 800, 1000, 
1600, 3200
 int pulseWidth = 411; //Opciones: 69, 118, 215, 411
 int adcRange = 4096; //Opciones: 2048, 4096, 8192, 16384
 // Configurar los parámetros deseados
115
 particleSensor.setup(ledBrightness, sampleAverage, ledMode,
sampleRate, pulseWidth, adcRange); // Configure el sensor con estos 
ajustes
}
double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100; //calcular SpO2 por este intervalo de muestreo
double ESpO2 = 93.0; // valor inicial de la SpO2 estimada
double FSpO2 = 0.7; // factor de filtro para la SpO2 estimada
double frate = 0.95; // filtro de paso bajo para valor de LED IR/rojo 
para eliminar el componente de CA
#define TIMETOBOOT 2000 // espere este tiempo (mseg) para generar SpO2
#define SAMPLING 5 // si desea ver los latidos del corazón con mayor 
precisión, configure MUESTREO en 1
#define FINGER_ON 35000 // si la señal roja es más baja que esto, 
indica que su dedo no está en el sensor
#define MINIMUM_SPO2 0.0
void loop() {
 
 if (WiFi.status() != WL_CONNECTED) {
 reconnect();
 }
 if (!tb.connected()) {
 // Connect to the ThingsBoard
 Serial.print("Conectando a: ");
 Serial.print(THINGSBOARD_SERVER);
 Serial.print(" con token ");
 Serial.println(TOKEN);
 if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
 Serial.println("Error en la conexión");
 return;
 }
 }
 uint32_t ir, red , green;
 double fred, fir;
 double SpO2 = 0; //// SpO2 antes de filtrado de paso bajo
 
 #ifdef USEFIFO
 particleSensor.check(); // Verifique el sensor, lea hasta 3 
muestras
 while (particleSensor.available()) { // tenemos nuevos datos
 #ifdef MAX30105
 red = particleSensor.getFIFORed(); //Sparkfun's MAX30105 
 ir = particleSensor.getFIFOIR(); //Sparkfun's MAX30105
 #else
 red = particleSensor.getFIFOIR();
 ir = particleSensor.getFIFORed();
 #endif
 i++;
 fred = (double)red;
 fir = (double)ir;
116
 avered = avered * frate + (double)red * (1.0 - frate);// nivel 
promedio de rojo por filtro de paso bajo
 aveir = aveir * frate + (double)ir * (1.0 - frate); // nivel de IR 
promedio por filtro de paso bajo
 sumredrms += (fred - avered) * (fred - avered); // suma cuadrada 
del componente alternativo del nivel rojo
 sumirrms += (fir - aveir) * (fir - aveir);// suma cuadrada del 
componente alternativo del nivel de IR 
 
 if ((i % SAMPLING) == 0){ // ralentizar la velocidad de trazado 
del gráfico para el trazador serial arduino por adelgazamiento
 if ( millis() > TIMETOBOOT) {
 if (ir < FINGER_ON) ESpO2 = MINIMUM_SPO2; // indicador de dedo 
desprendido
 if (ESpO2 <= -1){
 ESpO2 = 0;
 }
 if (ESpO2 > 100){
 ESpO2 = 100;
 }
 oxi = ESpO2;
 }
 }
 
 if ((i % Num) == 0) {
 double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) /
aveir);
 SpO2 = -23.3 * (R - 0.4) + 100;
//http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
 ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;//filtro de paso 
bajo
 sumredrms = 0.0; sumirrms = 0.0; i = 0;
 break;
 }
 particleSensor.nextSample(); // Hemos terminado con esta muestra, 
así que pase a la siguiente.
 //Serial.println(SpO2);
 
 long irHR = particleSensor.getIR();
 if(irHR > 7000){ // Si se 
detecta un dedo
 Serial.print("Nivel de SpO2: ");
 Serial.print(oxi);
 Serial.println(" %");
 Serial.print("Ritmo Cardiaco: ");
 Serial.print(mejorpromedio);
 Serial.println(" bpm");
 if (oxi <= 100 && oxi >= 80){
 tb.sendTelemetryInt("SpO2", oxi);
 tb.loop();
 Serial.println("Enviando SpO2...");
 }
 
 if (checkForBeat(irHR) == true){ //Si se 
detecta un latido del corazón
 if (mejorpromedio <= 200 && mejorpromedio >= 50){
 tb.sendTelemetryInt("Heart-rate", mejorpromedio);
 tb.loop();
 Serial.println("Enviando HR...");
 }
 
117
 delay(100);
 long delta = millis() - ultimolatido; //Medir 
la duración entre dos latidos
 ultimolatido = millis();
 latidosporminutos = 60/(delta/1000.0); //Calculando 
el BPM
 
 if (latidosporminutos < 255 && latidosporminutos > 20){
 rates[rateSpot++] = (byte)latidosporminutos; //Guarde esta 
lectura en el array
 rateSpot %= RATE_SIZE; //Ajustar variable
 //Tomar promedio de lecturas
 mejorpromedio = 0;
 for (byte x = 0; x < RATE_SIZE; x++)
 mejorpromedio += rates[x];
 mejorpromedio /= RATE_SIZE;
 }
 }
 }
 
 if (irHR < 7000){ //Si no se detecta ningún dedo, informe al 
usuario y ponga el BPM promedio a 0 o se almacenará para la siguiente 
medida
 mejorpromedio=0; 
 Serial.println("Error! No es posible detectar el dedo.");
 }
 }
 #endif
}
////////////// Functions ///////////////
void InitWiFi()
{
 Serial.println("Conectando al AP ...");
 // attempt to connect to WiFi network
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.println("Conectado al AP!");
}
void reconnect() {
 // Loop until we're reconnected
 status = WiFi.status();
 if ( status != WL_CONNECTED) {
 WiFi.begin(WIFI_AP, WIFI_PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.println("Conectado al AP");
 }
}

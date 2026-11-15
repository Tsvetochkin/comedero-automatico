#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

#define PIN_SERVO        13
#define PIN_TRIG         5
#define PIN_ECHO         18
#define PIN_BUZZER       25
#define PIN_BTN_MODO     14
#define PIN_BTN_MANUAL   27

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  32
Adafruit_SSD1306 oled(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);
Servo servo;

enum Modo { AUTOMATICO, MANUAL, PAUSA };
Modo modoActual = AUTOMATICO;

unsigned long ultimoDispensado = 0;
const unsigned long INTERVALO_AUTO = 10000;
const int UMBRAL_BAJO = 15;
const int ANGULO_ABIERTO = 90;
const int TIEMPO_ABIERTO = 800;

bool ultimoEstadoModo = HIGH;
bool ultimoEstadoManual = HIGH;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_MODO, INPUT_PULLUP);
  pinMode(PIN_BTN_MANUAL, INPUT_PULLUP);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  servo.attach(PIN_SERVO);
  servo.write(0);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED no encontrada");
  }
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay();
  mostrarLinea(0, "Comedero");
  mostrarLinea(1, "ESP32");
  delay(1500);

  actualizarPantalla();
}

void loop() {
  leerBotones();
  controlarDispensado();
  verificarNivel();
  delay(50);
}

void mostrarLinea(int fila, String texto) {
  oled.fillRect(0, fila * 16, ANCHO_PANTALLA, 16, SSD1306_BLACK);
  oled.setCursor(0, fila * 16);
  oled.print(texto);
  oled.display();
}

bool botonPulsado(int pin, bool &ultimoEstado) {
  bool estado = digitalRead(pin);
  bool pulsado = (ultimoEstado == HIGH && estado == LOW);
  ultimoEstado = estado;
  return pulsado;
}

void leerBotones() {
  if (botonPulsado(PIN_BTN_MODO, ultimoEstadoModo)) {
    modoActual = (Modo)((modoActual + 1) % 3);
    actualizarPantalla();
    delay(200);
  }

  if (botonPulsado(PIN_BTN_MANUAL, ultimoEstadoManual) && modoActual == MANUAL) {
    dispensarPorcion(ANGULO_ABIERTO, TIEMPO_ABIERTO);
    delay(200);
  }
}

void controlarDispensado() {
  if (modoActual == AUTOMATICO) {
    if (millis() - ultimoDispensado >= INTERVALO_AUTO) {
      dispensarPorcion(ANGULO_ABIERTO, TIEMPO_ABIERTO);
      ultimoDispensado = millis();
    }
  }
}

void dispensarPorcion(int angulo, int duracionMs) {
  mostrarLinea(1, "Sirviendo");

  servo.write(angulo);
  delay(duracionMs);
  servo.write(0);

  mostrarLinea(1, "Listo");
  delay(800);
  actualizarPantalla();
}

int medirDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracion == 0) return 999;
  return duracion * 0.034 / 2;
}

void activarAlarma(bool activa) {
  digitalWrite(PIN_BUZZER, activa ? HIGH : LOW);
}

void verificarNivel() {
  static unsigned long ultimaMedicion = 0;
  if (millis() - ultimaMedicion < 1500) return;
  ultimaMedicion = millis();

  int distancia = medirDistancia();
  Serial.print("Distancia: ");
  Serial.println(distancia);

  bool bajo = distancia > UMBRAL_BAJO;
  activarAlarma(bajo);
  mostrarLinea(1, bajo ? "Nivel BAJO" : "Nivel OK");
}

void actualizarPantalla() {
  switch (modoActual) {
    case AUTOMATICO: mostrarLinea(0, "AUTOMATICO"); break;
    case MANUAL:     mostrarLinea(0, "MANUAL"); break;
    case PAUSA:      mostrarLinea(0, "PAUSA"); break;
  }
  mostrarLinea(1, "Listo");
}

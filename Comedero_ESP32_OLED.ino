// Comedero automático programable - versión ESP32 con pantalla OLED
// Modos: AUTOMATICO (dispensa cada cierto tiempo), MANUAL (con el pulsador) y PAUSA.
// Un sensor ultrasónico mide el nivel de alimento y avisa con el zumbador si queda poco.

// ===== Librerías =====
#include <Wire.h>              // Comunicación I2C con la pantalla
#include <Adafruit_GFX.h>      // Funciones gráficas para dibujar texto en la OLED
#include <Adafruit_SSD1306.h>  // Driver de la pantalla OLED SSD1306
#include <ESP32Servo.h>        // Control del servomotor en el ESP32

// ===== Pines =====
#define PIN_SERVO        13    // Señal del servo (compuerta)
#define PIN_TRIG         5     // TRIG del HC-SR04 (envía el pulso)
#define PIN_ECHO         18    // ECHO del HC-SR04 (recibe el eco, con divisor de tensión)
#define PIN_BUZZER       25    // Zumbador de aviso
#define PIN_BTN_MODO     14    // Pulsador para cambiar de modo
#define PIN_BTN_MANUAL   27    // Pulsador para dispensar en modo manual

// ===== Pantalla y servo =====
#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  64
Adafruit_SSD1306 oled(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1); // -1: sin pin de reset
Servo servo;

// ===== Modos de funcionamiento =====
enum Modo { AUTOMATICO, MANUAL, PAUSA };
Modo modoActual = AUTOMATICO;  // Arranca en automático

// ===== Variables y constantes =====
unsigned long ultimoDispensado = 0;          // Momento (ms) de la última porción automática
const unsigned long INTERVALO_AUTO = 10000;  // 10 s para la prueba, en uso real serían horas
const int UMBRAL_BAJO = 15;                  // Si la distancia supera 15 cm, hay poco alimento
const int ANGULO_ABIERTO = 90;               // Ángulo del servo con la compuerta abierta
const int TIEMPO_ABIERTO = 800;              // Tiempo (ms) que la compuerta queda abierta

// Último estado leído de cada pulsador, para detectar solo el momento en que se presiona
bool ultimoEstadoModo = HIGH;
bool ultimoEstadoManual = HIGH;

void setup() {
  Serial.begin(115200);  // Monitor serie para ver la distancia medida

  // Pulsadores con resistencia pull-up interna: leen LOW al presionarlos
  pinMode(PIN_BTN_MODO, INPUT_PULLUP);
  pinMode(PIN_BTN_MANUAL, INPUT_PULLUP);

  // Sensor ultrasónico y zumbador
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);  // Zumbador apagado al iniciar

  // Servo: la compuerta empieza cerrada
  servo.attach(PIN_SERVO);
  servo.write(0);

  // Pantalla OLED en la dirección I2C 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED no encontrada");
  }
  oled.setTextSize(2);              // Texto grande: entran 2 líneas
  oled.setTextColor(SSD1306_WHITE);
  oled.clearDisplay();
  mostrarLinea(0, "Comedero");      // Mensaje de inicio
  mostrarLinea(1, "ESP32");
  delay(1500);

  actualizarPantalla();             // Muestra el modo inicial
}

void loop() {
  leerBotones();          // 1. Revisa los pulsadores (cambio de modo o porción manual)
  controlarDispensado();  // 2. En modo automático, dispensa cuando pasó el intervalo
  verificarNivel();       // 3. Mide el nivel de alimento y activa el aviso si hace falta
  delay(50);              // Pequeña pausa para no saturar el ciclo
}

// Escribe un texto en la fila indicada (0 = arriba, 1 = abajo) borrando lo anterior
void mostrarLinea(int fila, String texto) {
  oled.fillRect(0, fila * 16, ANCHO_PANTALLA, 16, SSD1306_BLACK); // Borra solo esa fila
  oled.setCursor(0, fila * 16);
  oled.print(texto);
  oled.display();
}

// Devuelve true solo en el instante en que el pulsador pasa de suelto (HIGH) a presionado (LOW)
bool botonPulsado(int pin, bool &ultimoEstado) {
  bool estado = digitalRead(pin);
  bool pulsado = (ultimoEstado == HIGH && estado == LOW);
  ultimoEstado = estado;  // Guarda el estado para la próxima vuelta
  return pulsado;
}

// Lee los dos pulsadores y actúa según el modo
void leerBotones() {
  // Pulsador de modo: AUTOMATICO -> MANUAL -> PAUSA -> AUTOMATICO
  if (botonPulsado(PIN_BTN_MODO, ultimoEstadoModo)) {
    modoActual = (Modo)((modoActual + 1) % 3);
    actualizarPantalla();
    delay(200);  // Antirrebote
  }

  // Pulsador manual: solo dispensa si estamos en modo MANUAL
  if (botonPulsado(PIN_BTN_MANUAL, ultimoEstadoManual) && modoActual == MANUAL) {
    dispensarPorcion(ANGULO_ABIERTO, TIEMPO_ABIERTO);
    delay(200);  // Antirrebote
  }
}

// En modo automático, dispensa una porción cada INTERVALO_AUTO milisegundos
void controlarDispensado() {
  if (modoActual == AUTOMATICO) {
    // millis() permite medir el tiempo sin frenar el programa con delay()
    if (millis() - ultimoDispensado >= INTERVALO_AUTO) {
      dispensarPorcion(ANGULO_ABIERTO, TIEMPO_ABIERTO);
      ultimoDispensado = millis();
    }
  }
}

// Abre la compuerta al ángulo indicado durante duracionMs y la vuelve a cerrar
void dispensarPorcion(int angulo, int duracionMs) {
  mostrarLinea(1, "Sirviendo");

  servo.write(angulo);  // Abre
  delay(duracionMs);    // Deja caer el alimento
  servo.write(0);       // Cierra

  mostrarLinea(1, "Listo");
  delay(800);
  actualizarPantalla();
}

// Mide la distancia en cm entre el sensor y el alimento
int medirDistancia() {
  // Pulso de 10 microsegundos en TRIG para iniciar la medición
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Tiempo que tarda en volver el eco (máximo 30 ms)
  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracion == 0) return 999;  // Sin eco: lo tomamos como depósito vacío
  return duracion * 0.034 / 2;    // Velocidad del sonido 0,034 cm/us, ida y vuelta
}

// Enciende o apaga el zumbador
void activarAlarma(bool activa) {
  digitalWrite(PIN_BUZZER, activa ? HIGH : LOW);
}

// Cada 1,5 s mide el nivel y avisa si hay poco alimento
void verificarNivel() {
  static unsigned long ultimaMedicion = 0;  // static: conserva el valor entre llamadas
  if (millis() - ultimaMedicion < 1500) return;
  ultimaMedicion = millis();

  int distancia = medirDistancia();
  Serial.print("Distancia: ");
  Serial.println(distancia);

  // Más distancia = menos alimento en el depósito
  bool bajo = distancia > UMBRAL_BAJO;
  activarAlarma(bajo);
  mostrarLinea(1, bajo ? "Nivel BAJO" : "Nivel OK");
}

// Muestra el modo actual en la fila de arriba
void actualizarPantalla() {
  switch (modoActual) {
    case AUTOMATICO: mostrarLinea(0, "AUTOMATICO"); break;
    case MANUAL:     mostrarLinea(0, "MANUAL"); break;
    case PAUSA:      mostrarLinea(0, "PAUSA"); break;
  }
  mostrarLinea(1, "Listo");
}

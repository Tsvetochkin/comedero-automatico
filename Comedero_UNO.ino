// Comedero automático programable - versión Arduino UNO con LCD 16x2
// Modos: AUTOMATICO (dispensa cada cierto tiempo), MANUAL (con el pulsador) y PAUSA.
// Un sensor ultrasónico mide el nivel de alimento y avisa con el zumbador si queda poco.

// ===== Librerías =====
#include <Wire.h>                    // Comunicación I2C
#include <Adafruit_LiquidCrystal.h>  // Pantalla LCD por I2C (la que usa Tinkercad)
#include <Servo.h>                   // Control del servomotor

// ===== Pines =====
#define PIN_SERVO        9   // Señal del servo (compuerta)
#define PIN_TRIG         7   // TRIG del HC-SR04 (envía el pulso)
#define PIN_ECHO         6   // ECHO del HC-SR04 (recibe el eco)
#define PIN_BUZZER       8   // Zumbador de aviso
#define PIN_BTN_MODO     3   // Pulsador para cambiar de modo
#define PIN_BTN_MANUAL   2   // Pulsador para dispensar en modo manual

// ===== Pantalla y servo =====
Adafruit_LiquidCrystal lcd(0);  // LCD con dirección I2C 0
Servo servo;

// ===== Modos de funcionamiento =====
enum Modo { AUTOMATICO, MANUAL, PAUSA };
Modo modoActual = AUTOMATICO;  // Arranca en automático

// ===== Variables y constantes =====
unsigned long ultimoDispensado = 0;          // Momento (ms) de la última porción automática
const unsigned long INTERVALO_AUTO = 10000;  // 10 s para la prueba, en uso real serían horas
const int UMBRAL_BAJO = 15;                  // Si la distancia supera 15 cm, hay poco alimento

// Último estado leído de cada pulsador, para detectar solo el momento en que se presiona
bool ultimoEstadoModo = HIGH;
bool ultimoEstadoManual = HIGH;

void setup() {
  Serial.begin(9600);  // Monitor serie para ver la distancia medida

  // Pulsadores con resistencia pull-up interna: leen LOW al presionarlos
  pinMode(PIN_BTN_MODO, INPUT_PULLUP);
  pinMode(PIN_BTN_MANUAL, INPUT_PULLUP);

  // Sensor ultrasónico y zumbador
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Servo: la compuerta empieza cerrada
  servo.attach(PIN_SERVO);
  servo.write(0);

  // LCD de 16 columnas y 2 filas con mensaje de inicio
  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Comedero UNO");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1500);

  actualizarPantalla();  // Muestra el modo inicial
}

void loop() {
  leerBotones();          // 1. Revisa los pulsadores (cambio de modo o porción manual)
  controlarDispensado();  // 2. En modo automático, dispensa cuando pasó el intervalo
  verificarNivel();       // 3. Mide el nivel de alimento y activa el aviso si hace falta
  delay(50);              // Pequeña pausa para no saturar el ciclo
}

// Lee los dos pulsadores y actúa solo en el instante en que se presionan (HIGH -> LOW)
void leerBotones() {
  bool estadoModo = digitalRead(PIN_BTN_MODO);
  bool estadoManual = digitalRead(PIN_BTN_MANUAL);

  // Pulsador de modo: AUTOMATICO -> MANUAL -> PAUSA -> AUTOMATICO
  if (ultimoEstadoModo == HIGH && estadoModo == LOW) {
    modoActual = (Modo)((modoActual + 1) % 3);
    actualizarPantalla();
    delay(200);  // Antirrebote
  }
  ultimoEstadoModo = estadoModo;

  // Pulsador manual: solo dispensa si estamos en modo MANUAL
  if (modoActual == MANUAL && ultimoEstadoManual == HIGH && estadoManual == LOW) {
    dispensarPorcion();
    delay(200);  // Antirrebote
  }
  ultimoEstadoManual = estadoManual;
}

// En modo automático, dispensa una porción cada INTERVALO_AUTO milisegundos
void controlarDispensado() {
  if (modoActual == AUTOMATICO) {
    // millis() permite medir el tiempo sin frenar el programa con delay()
    if (millis() - ultimoDispensado >= INTERVALO_AUTO) {
      dispensarPorcion();
      ultimoDispensado = millis();
    }
  }
}

// Abre la compuerta 90 grados durante 800 ms y la vuelve a cerrar
void dispensarPorcion() {
  lcd.setCursor(0, 1);
  lcd.print("Dispensando...  ");

  servo.write(90);  // Abre
  delay(800);       // Deja caer el alimento
  servo.write(0);   // Cierra

  lcd.setCursor(0, 1);
  lcd.print("Listo           ");
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

// Cada 1,5 s mide el nivel y avisa si hay poco alimento
void verificarNivel() {
  static unsigned long ultimaMedicion = 0;  // static: conserva el valor entre llamadas
  if (millis() - ultimaMedicion < 1500) return;
  ultimaMedicion = millis();

  int distancia = medirDistancia();
  Serial.print("Distancia: ");
  Serial.println(distancia);

  // Más distancia = menos alimento en el depósito
  if (distancia > UMBRAL_BAJO) {
    tone(PIN_BUZZER, 1000);  // Suena a 1000 Hz
    lcd.setCursor(0, 1);
    lcd.print("Comida: BAJA    ");
  } else {
    noTone(PIN_BUZZER);      // Silencio
    lcd.setCursor(0, 1);
    lcd.print("Comida: OK      ");
  }
}

// Muestra el modo actual en la fila de arriba (espacios al final para borrar texto viejo)
void actualizarPantalla() {
  lcd.clear();
  lcd.setCursor(0, 0);

  switch (modoActual) {
    case AUTOMATICO: lcd.print("Modo: AUTOMATICO"); break;
    case MANUAL:     lcd.print("Modo: MANUAL    "); break;
    case PAUSA:      lcd.print("Modo: PAUSA     "); break;
  }

  lcd.setCursor(0, 1);
  lcd.print("Listo           ");
}

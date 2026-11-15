#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>
#include <Servo.h>

#define PIN_SERVO        9
#define PIN_TRIG         7
#define PIN_ECHO         6
#define PIN_BUZZER       8
#define PIN_BTN_MODO     3
#define PIN_BTN_MANUAL   2

Adafruit_LiquidCrystal lcd(0);
Servo servo;

enum Modo { AUTOMATICO, MANUAL, PAUSA };
Modo modoActual = AUTOMATICO;

unsigned long ultimoDispensado = 0;
const unsigned long INTERVALO_AUTO = 10000;
const int UMBRAL_BAJO = 15;

bool ultimoEstadoModo = HIGH;
bool ultimoEstadoManual = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_BTN_MODO, INPUT_PULLUP);
  pinMode(PIN_BTN_MANUAL, INPUT_PULLUP);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  servo.attach(PIN_SERVO);
  servo.write(0);

  lcd.begin(16, 2);
  lcd.setBacklight(1);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Comedero UNO");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1500);

  actualizarPantalla();
}

void loop() {
  leerBotones();
  controlarDispensado();
  verificarNivel();
  delay(50);
}

void leerBotones() {
  bool estadoModo = digitalRead(PIN_BTN_MODO);
  bool estadoManual = digitalRead(PIN_BTN_MANUAL);

  if (ultimoEstadoModo == HIGH && estadoModo == LOW) {
    modoActual = (Modo)((modoActual + 1) % 3);
    actualizarPantalla();
    delay(200);
  }
  ultimoEstadoModo = estadoModo;

  if (modoActual == MANUAL && ultimoEstadoManual == HIGH && estadoManual == LOW) {
    dispensarPorcion();
    delay(200);
  }
  ultimoEstadoManual = estadoManual;
}

void controlarDispensado() {
  if (modoActual == AUTOMATICO) {
    if (millis() - ultimoDispensado >= INTERVALO_AUTO) {
      dispensarPorcion();
      ultimoDispensado = millis();
    }
  }
}

void dispensarPorcion() {
  lcd.setCursor(0, 1);
  lcd.print("Dispensando...  ");

  servo.write(90);
  delay(800);
  servo.write(0);

  lcd.setCursor(0, 1);
  lcd.print("Listo           ");
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

void verificarNivel() {
  static unsigned long ultimaMedicion = 0;
  if (millis() - ultimaMedicion < 1500) return;
  ultimaMedicion = millis();

  int distancia = medirDistancia();
  Serial.print("Distancia: ");
  Serial.println(distancia);

  if (distancia > UMBRAL_BAJO) {
    tone(PIN_BUZZER, 1000);
    lcd.setCursor(0, 1);
    lcd.print("Comida: BAJA    ");
  } else {
    noTone(PIN_BUZZER);
    lcd.setCursor(0, 1);
    lcd.print("Comida: OK      ");
  }
}

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

# Comedero automático programable

Proyecto del Segundo Parcial de Integración Tecnológica (Escuela Da Vinci).

Comedero para mascotas que dispensa alimento con un servomotor. Tiene tres modos (automático, manual y pausa), mide el nivel del depósito con un sensor ultrasónico y avisa con un zumbador y un mensaje en pantalla cuando queda poco alimento.

## Versiones

| Archivo | Placa | Pantalla | Simulación |
|---|---|---|---|
| `Comedero_UNO.ino` | Arduino UNO | LCD 16x2 I2C | [Tinkercad](https://www.tinkercad.com/things/bkTBLtPfgUi-spectacular-jaagub/editel?sharecode=qkklTK2UCeEy1hw_tv5r9D9wAsKG68oIUkh7F0Qf6p4) |
| `Comedero_ESP32_OLED.ino` | ESP32 | OLED 128x32 SSD1306 | [Wokwi](https://wokwi.com/projects/480783610547213313) |

## Componentes

- Placa Arduino UNO o ESP32
- Servomotor (compuerta)
- Sensor ultrasónico HC-SR04 (nivel de alimento)
- Pantalla LCD 16x2 I2C (UNO) u OLED SSD1306 (ESP32)
- 2 pulsadores (cambio de modo y porción manual)
- Zumbador
- Resistencias para el divisor de tensión del ECHO (versión ESP32)

## Funcionamiento

- **Pulsador de modo:** cambia entre AUTOMATICO, MANUAL y PAUSA.
- **AUTOMATICO:** dispensa una porción cada 10 segundos (intervalo de prueba).
- **MANUAL:** dispensa una porción cada vez que se presiona el pulsador manual.
- **PAUSA:** no dispensa.
- **Nivel bajo:** si el sensor mide más de 15 cm hasta el alimento, suena el zumbador y la pantalla muestra el aviso.

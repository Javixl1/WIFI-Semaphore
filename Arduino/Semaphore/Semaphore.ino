/*
 * Trafic Light
 */
// Asignación de pines
const int LightGreen =  17;
const int LightYellow = 20;
const int LightRed =    19;
const int Lightwhite =  18;
// Variable para el estado del semáforo
int state = 0;
unsigned long previousMillis = 0;
const long interval = 1000; // 1 segundo

void setup() {
  pinMode(LightGreen, OUTPUT);
  pinMode(LightYellow, OUTPUT);
  pinMode(LightRed, OUTPUT);
  pinMode(Lightwhite, OUTPUT);
  }

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    switch (state) {
      case 0: // Verde
        digitalWrite(LightGreen, HIGH);
        digitalWrite(LightYellow, LOW);
        digitalWrite(LightRed, LOW);
        digitalWrite(Lightwhite, LOW);
        break;

      case 1: // Amarillo
        digitalWrite(LightGreen, LOW);
        digitalWrite(LightYellow, HIGH);
        digitalWrite(LightRed, LOW);
        digitalWrite(Lightwhite, LOW);
        break;

      case 2: // Rojo
        digitalWrite(LightGreen, LOW);
        digitalWrite(LightYellow, LOW);
        digitalWrite(LightRed, HIGH);
        digitalWrite(Lightwhite, LOW);
        break;

      case 3: // Rojo
        digitalWrite(LightGreen, LOW);
        digitalWrite(LightYellow, LOW);
        digitalWrite(LightRed, LOW);
        digitalWrite(Lightwhite, HIGH);
        break;
        }

    // Avanzar al siguiente estado
    state = (state + 1) % 4;
  }
}


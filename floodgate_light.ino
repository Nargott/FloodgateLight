#include <Adafruit_NeoPixel.h>
#include <InputDebounce.h>

#define BUTTON_DEBOUNCE_DELAY   20   // [ms]
#define MODES_COUNT 3
#define BLINK_DELAY_MS 200

enum MODES {
  MODE_REACT_BLINK = 0,
  MODE_REACT_SHINE = 1,
  MODE_CONST_SHINE = 2
};

enum LIGHT_PINS {
  LR_PIN = 3,
  LG_PIN = 2,
  LB_PIN = 4,
  LN_PIN = 5 //reserved
};

enum RADIO_PINS {
  BA_PIN = 8,
  BB_PIN = 6,
  BC_PIN = 9,
  BD_PIN = 7
};

enum PIR_PINS {
  PIR1_PIN = 10,
  PIR2_PIN = 11
};

struct Mode {
  byte R;
  byte G;
  byte B;
  byte index;
  unsigned long lastUpdated;
};

const int LED_PIN = 12;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static InputDebounce radioButtonA;
static InputDebounce radioButtonB;
static InputDebounce radioButtonC;
static InputDebounce radioButtonD;

static InputDebounce PIRSensor1;
static InputDebounce PIRSensor2;

Mode currentMode;
boolean PIRSensorActivated = false;

void updateStatusLED(boolean turnON = true) {
  if (!turnON) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  } else {
    pixels.setPixelColor(0, pixels.Color(currentMode.R, currentMode.G, currentMode.B));
  }
  pixels.show();
}

void updateMainLight(boolean turnON = true) {
  if (!turnON) {
    digitalWrite(LG_PIN, HIGH);
    digitalWrite(LB_PIN, HIGH);
    digitalWrite(LR_PIN, HIGH);
    return;
  }
  if (currentMode.R > 0) {
    digitalWrite(LR_PIN, LOW);
  } else {
    digitalWrite(LR_PIN, HIGH);
  }
  if (currentMode.G > 0) {
    digitalWrite(LG_PIN, LOW);
  } else {
    digitalWrite(LG_PIN, HIGH);
  }
  if (currentMode.B > 0) {
    digitalWrite(LB_PIN, LOW);
  } else {
    digitalWrite(LB_PIN, HIGH);
  }
}

void radioButtonPressed_cb() {
  Serial.println("radioButtonPressed_cb");
  if (digitalRead(BA_PIN) == HIGH) {
    currentMode.R = (currentMode.R == 0) ? 255 : 0;
  }
  if (digitalRead(BB_PIN) == HIGH) {
    currentMode.G = (currentMode.G == 0) ? 255 : 0;
  }
  if (digitalRead(BC_PIN) == HIGH) {
    currentMode.B = (currentMode.B == 0) ? 255 : 0;
  }
  if (digitalRead(BD_PIN) == HIGH) {
    currentMode.index = (currentMode.index < MODES_COUNT) ? (currentMode.index + 1) : 0;
  }

  currentMode.lastUpdated = millis();
  updateStatusLED();
}

void PIRActivated_cb() {
  if (digitalRead(PIR1_PIN) == HIGH) Serial.println("PIR1 on");
  if (digitalRead(PIR2_PIN) == HIGH) Serial.println("PIR2 on");
  PIRSensorActivated = true;
}

void PIRDisactivated_cb() {
  Serial.println("PIRs off");
  PIRSensorActivated = false;
}

void setup() {
  //RGB strip relays setup
  pinMode(LR_PIN, OUTPUT);
  pinMode(LG_PIN, OUTPUT);
  pinMode(LB_PIN, OUTPUT);
  pinMode(LN_PIN, OUTPUT);
  digitalWrite(LG_PIN, HIGH);
  digitalWrite(LB_PIN, HIGH);
  digitalWrite(LR_PIN, HIGH);
  digitalWrite(LN_PIN, HIGH);

  Serial.begin(9600);
  pixels.begin();

  //radio buttons setup
  radioButtonA.registerCallbacks(radioButtonPressed_cb, NULL, NULL);
  radioButtonB.registerCallbacks(radioButtonPressed_cb, NULL, NULL);
  radioButtonC.registerCallbacks(radioButtonPressed_cb, NULL, NULL);
  radioButtonD.registerCallbacks(radioButtonPressed_cb, NULL, NULL);
  radioButtonA.setup(BA_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  radioButtonC.setup(BB_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  radioButtonB.setup(BC_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  radioButtonD.setup(BD_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);

  //PIR sensors setup
  PIRSensor1.registerCallbacks(PIRActivated_cb, PIRDisactivated_cb, NULL);
  PIRSensor2.registerCallbacks(PIRActivated_cb, PIRDisactivated_cb, NULL);
  PIRSensor1.setup(PIR1_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  PIRSensor2.setup(PIR2_PIN, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_EXT_PULL_DOWN_RES);

  Serial.println("Hello!");

  currentMode = {255, 0, 0, 0, millis()};

  updateStatusLED();
}

unsigned long now;
boolean blinkState = true;
unsigned long blinkedLastTime = 0;
unsigned long lastChekingModeTime;
void loop() {
  now = millis();
  radioButtonA.process(now);
  radioButtonB.process(now);
  radioButtonC.process(now);
  radioButtonD.process(now);

  PIRSensor1.process(now);
  PIRSensor2.process(now);

  switch (currentMode.index) {
    case MODE_REACT_BLINK: {
        if (now - blinkedLastTime > BLINK_DELAY_MS) {
          if (blinkState) {
            updateStatusLED();
            if (PIRSensorActivated) updateMainLight();
            lastChekingModeTime = now;
          } else {
            updateStatusLED(false);
            if (PIRSensorActivated) updateMainLight(false);
            lastChekingModeTime = now;
          }
          blinkState = !blinkState;
          blinkedLastTime = now;
        }
        if ((!PIRSensorActivated) && (lastChekingModeTime != currentMode.lastUpdated)) {
          if (!PIRSensorActivated) updateMainLight(false);
          lastChekingModeTime = currentMode.lastUpdated;
        }
      } break;

    case MODE_REACT_SHINE: {
        if ((blinkState != PIRSensorActivated) || (lastChekingModeTime != currentMode.lastUpdated)) {
          (PIRSensorActivated) ? updateMainLight() : updateMainLight(false);
          blinkState = PIRSensorActivated;
          lastChekingModeTime = currentMode.lastUpdated;
        }
      } break;

    case MODE_CONST_SHINE: {
        if (lastChekingModeTime != currentMode.lastUpdated) {
          updateStatusLED();
          updateMainLight();
          lastChekingModeTime = currentMode.lastUpdated;
        }
      } break;
  }
}

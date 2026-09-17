#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Screen Settings ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// --- Pin Definitions (Matching Veroboard Layout) ---
#define I2C_SDA   13
#define I2C_SCL   14

#define PUMP1_IN1 32
#define PUMP1_IN2 33
#define PUMP2_IN3 25
#define PUMP2_IN4 26

#define BUTTON_PIN 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- System States ---
enum SystemState {
  STATE_IDLE,
  STATE_FLAP_EXTENDING,
  STATE_FILLING,
  STATE_SETTLING,
  STATE_ANALYSING,
  STATE_DRAINING,
  STATE_DRAIN_WAIT,
  STATE_RINSE1_FILL,
  STATE_RINSE1_WAIT1,
  STATE_RINSE1_DRAIN,
  STATE_RINSE1_WAIT2,
  STATE_RINSE2_FILL,
  STATE_RINSE2_WAIT1,
  STATE_RINSE2_DRAIN,
  STATE_COMPLETE
};

SystemState currentState = STATE_IDLE;
unsigned long stateStartTime = 0;

// --- Timing Configurations (in milliseconds) ---
const unsigned long FLAP_DURATION        = 5000;   // 5 seconds
const unsigned long FILL_DURATION        = 6000;   // 6 seconds (Both pumps run)
const unsigned long SETTLE_DURATION      = 3000;   // 3 seconds pause
const unsigned long ANALYSE_DURATION     = 15000;  // 15 seconds (SWV placeholder)
const unsigned long DRAIN_DURATION       = (FILL_DURATION * 2) + 2000; // 14s (Pump 1 REVERSE)
const unsigned long WAIT_DURATION        = 2000;   // 2 seconds pause
const unsigned long RINSE_FILL_DURATION  = 6000;   // 6 seconds (Buffer Pump 2)
const unsigned long RINSE_DRAIN_DURATION = 8000;   // 8 seconds (Drain Pump 1 REVERSE)
const unsigned long COMPLETE_HOLD        = 3000;   // 3 seconds display hold

// --- Motor Control Functions ---
void stopPumps() {
  analogWrite(PUMP1_IN1, 0);
  analogWrite(PUMP1_IN2, 0);
  analogWrite(PUMP2_IN3, 0);
  analogWrite(PUMP2_IN4, 0);
}

void runBothPumpsForward(uint8_t speed = 255) {
  analogWrite(PUMP1_IN1, speed);
  analogWrite(PUMP1_IN2, 0);
  analogWrite(PUMP2_IN3, speed);
  analogWrite(PUMP2_IN4, 0);
}

void runPump1Reverse(uint8_t speed = 255) {
  analogWrite(PUMP1_IN1, 0);
  analogWrite(PUMP1_IN2, speed);
  analogWrite(PUMP2_IN3, 0);
  analogWrite(PUMP2_IN4, 0);
}

void runPump2Forward(uint8_t speed = 255) {
  analogWrite(PUMP1_IN1, 0);
  analogWrite(PUMP1_IN2, 0);
  analogWrite(PUMP2_IN3, speed);
  analogWrite(PUMP2_IN4, 0);
}

// --- OLED Display Helper ---
void updateDisplay(const char* stateName, const char* statusMsg, unsigned long elapsed, unsigned long total) {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MICROFLUIDIC CONTROL");
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  // State Title
  display.setCursor(0, 16);
  display.print("STATE: ");
  display.println(stateName);

  // Status Message
  display.setCursor(0, 30);
  display.println(statusMsg);

  // Progress Bar
  if (total > 0) {
    int progress = map(constrain(elapsed, 0, total), 0, total, 0, 120);
    display.drawRect(4, 48, 120, 12, SSD1306_WHITE);
    display.fillRect(6, 50, progress, 8, SSD1306_WHITE);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(PUMP1_IN1, OUTPUT);
  pinMode(PUMP1_IN2, OUTPUT);
  pinMode(PUMP2_IN3, OUTPUT);
  pinMode(PUMP2_IN4, OUTPUT);
  stopPumps();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 OLED allocation failed"));
    for (;;);
  }

  updateDisplay("IDLE", "Press Button to Start", 0, 0);
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - stateStartTime;

  switch (currentState) {

    case STATE_IDLE:
      stopPumps();
      if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50); // Debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
          currentState = STATE_FLAP_EXTENDING;
          stateStartTime = millis();
        }
      }
      break;

    case STATE_FLAP_EXTENDING:
      stopPumps();
      updateDisplay("FLAP EXTEND", "Flap Extending...", elapsedTime, FLAP_DURATION);

      if (elapsedTime >= FLAP_DURATION) {
        currentState = STATE_FILLING;
        stateStartTime = millis();
      }
      break;

    case STATE_FILLING:
      runBothPumpsForward(255);
      updateDisplay("FILLING", "Fill & Buffer On...", elapsedTime, FILL_DURATION);

      if (elapsedTime >= FILL_DURATION) {
        stopPumps();
        currentState = STATE_SETTLING;
        stateStartTime = millis();
      }
      break;

    case STATE_SETTLING:
      stopPumps();
      updateDisplay("SETTLING", "Sample Resting...", elapsedTime, SETTLE_DURATION);

      if (elapsedTime >= SETTLE_DURATION) {
        currentState = STATE_ANALYSING;
        stateStartTime = millis();
      }
      break;

    case STATE_ANALYSING:
      stopPumps();
      updateDisplay("ANALYSING", "Analysing Sample...", elapsedTime, ANALYSE_DURATION);

      if (elapsedTime >= ANALYSE_DURATION) {
        currentState = STATE_DRAINING;
        stateStartTime = millis();
      }
      break;

    case STATE_DRAINING:
      runPump1Reverse(255);
      updateDisplay("DRAINING", "Draining Fluid...", elapsedTime, DRAIN_DURATION);

      if (elapsedTime >= DRAIN_DURATION) {
        stopPumps();
        currentState = STATE_DRAIN_WAIT;
        stateStartTime = millis();
      }
      break;

    case STATE_DRAIN_WAIT:
      stopPumps();
      updateDisplay("WAITING", "Pausing...", elapsedTime, WAIT_DURATION);

      if (elapsedTime >= WAIT_DURATION) {
        currentState = STATE_RINSE1_FILL;
        stateStartTime = millis();
      }
      break;

    // --- RINSE CYCLE 1 ---
    case STATE_RINSE1_FILL:
      runPump2Forward(255);
      updateDisplay("RINSE 1", "Buffer Fill...", elapsedTime, RINSE_FILL_DURATION);

      if (elapsedTime >= RINSE_FILL_DURATION) {
        stopPumps();
        currentState = STATE_RINSE1_WAIT1;
        stateStartTime = millis();
      }
      break;

    case STATE_RINSE1_WAIT1:
      stopPumps();
      updateDisplay("RINSE 1", "Pausing...", elapsedTime, WAIT_DURATION);

      if (elapsedTime >= WAIT_DURATION) {
        currentState = STATE_RINSE1_DRAIN;
        stateStartTime = millis();
      }
      break;

    case STATE_RINSE1_DRAIN:
      runPump1Reverse(255);
      updateDisplay("RINSE 1", "Draining Rinse...", elapsedTime, RINSE_DRAIN_DURATION);

      if (elapsedTime >= RINSE_DRAIN_DURATION) {
        stopPumps();
        currentState = STATE_RINSE1_WAIT2;
        stateStartTime = millis();
      }
      break;

    case STATE_RINSE1_WAIT2:
      stopPumps();
      updateDisplay("RINSE 1", "Pausing...", elapsedTime, WAIT_DURATION);

      if (elapsedTime >= WAIT_DURATION) {
        currentState = STATE_RINSE2_FILL;
        stateStartTime = millis();
      }
      break;

    // --- RINSE CYCLE 2 ---
    case STATE_RINSE2_FILL:
      runPump2Forward(255);
      updateDisplay("RINSE 2", "Buffer Fill...", elapsedTime, RINSE_FILL_DURATION);

      if (elapsedTime >= RINSE_FILL_DURATION) {
        stopPumps();
        currentState = STATE_RINSE2_WAIT1;
        stateStartTime = millis();
      }
      break;

    case STATE_RINSE2_WAIT1:
      stopPumps();
      updateDisplay("RINSE 2", "Pausing...", elapsedTime, WAIT_DURATION);

      if (elapsedTime >= WAIT_DURATION) {
        currentState = STATE_RINSE2_DRAIN;
        stateStartTime = millis();
      }
      break;

    case STATE_RINSE2_DRAIN:
      runPump1Reverse(255);
      updateDisplay("RINSE 2", "Draining Rinse...", elapsedTime, RINSE_DRAIN_DURATION);

      if (elapsedTime >= RINSE_DRAIN_DURATION) {
        stopPumps();
        currentState = STATE_COMPLETE;
        stateStartTime = millis();
      }
      break;

    case STATE_COMPLETE:
      stopPumps();
      updateDisplay("COMPLETE", "Cycle Complete!", 0, 0);

      if (elapsedTime >= COMPLETE_HOLD) {
        currentState = STATE_IDLE;
        updateDisplay("IDLE", "Press Button to Start", 0, 0);
      }
      break;
  }
}
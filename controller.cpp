#include <libs/GyverOLED/GyverOLED.h>
#include <libs/Encoder/Encoder.h>

#define ENC_A 2
#define ENC_B 3
#define ENC_BTN 11

// Пины реле (теперь только 5 штук, AUTO не использует реле)
const byte relayPins[5] = {4,5,6,7,8}; // 10M,15M,20M,40M,80M
bool relayState[5] = {0};

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
Encoder myEnc(ENC_A, ENC_B);

int mode = 0; // 0=AUTO,1=10M,2=15M,3=20M,4=40M,5=80M,6=AUTO
int lastActiveRelay = -1;
long lastProcessedClick = 0;

bool lastBtn = HIGH;
bool waiting = false;
unsigned long waitTimer = 0;

const char* const modes[] = {"AUTO","10M","15M","20M","40M","80M","AUTO"};

// Функция получения индекса реле по режиму
int getRelayIndex(int m) {
  // AUTO (0 и 6) не используют реле → возвращаем -1
  if (m == 0 || m == 6) return -1;
  // 10M(1)->0, 15M(2)->1, 20M(3)->2, 40M(4)->3, 80M(5)->4
  return m - 1;
}

void setup() {
  pinMode(ENC_BTN, INPUT_PULLUP);
  for (int i=0; i<5; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }
  
  oled.init();
  oled.setScale(4);
  lastProcessedClick = myEnc.read() / 4;
  updateDisplay();
}

void loop() {
  // ===== ЭНКОДЕР =====
  long currentClick = myEnc.read() / 4;
  if (currentClick != lastProcessedClick) {
    if (currentClick > lastProcessedClick) {
      if (mode < 6) mode++;
    } else {
      if (mode > 0) mode--;
    }
    updateDisplay();
    lastProcessedClick = currentClick;
  }
  
  // ===== ОБРАБОТКА ЗАДЕРЖКИ =====
  if (waiting && millis() - waitTimer >= 100) {
    int newRelay = getRelayIndex(mode);
    // Включаем новое реле только если это не AUTO
    if (newRelay >= 0) {
      relayState[newRelay] = true;
      digitalWrite(relayPins[newRelay], LOW);
    }
    waiting = false;
    updateDisplay();
  }
  
  // ===== КНОПКА =====
  bool btn = digitalRead(ENC_BTN);
  if (btn == LOW && lastBtn == HIGH && !waiting) {
    
    int newRelay = getRelayIndex(mode);
    
    // Выключаем предыдущее активное реле (если было)
    if (lastActiveRelay >= 0 && lastActiveRelay < 5) {
      relayState[lastActiveRelay] = false;
      digitalWrite(relayPins[lastActiveRelay], HIGH);
    }
    
    // Запоминаем новое реле (даже если -1, тогда следующее нажатие ничего не выключит)
    lastActiveRelay = newRelay;
    
    // Запускаем таймер для включения нового (только если это не AUTO)
    if (newRelay >= 0) {
      waiting = true;
      waitTimer = millis();
    }
    
    updateDisplay();
    delay(50);
  }
  lastBtn = btn;
}

void updateDisplay() {
  oled.clear();
  oled.setCursorXY(28, 16);
  oled.print(modes[mode]);
  
  oled.setScale(1);
  oled.setCursorXY(0, 45);
  oled.print("10 15 20 40 80"); // подписи под реле
  oled.setCursorXY(0, 53);
  // Показываем состояние только для 5 реле
  for (int i=0; i<5; i++) oled.print(relayState[i] ? "1  " : "0  ");
  oled.setScale(4);
}

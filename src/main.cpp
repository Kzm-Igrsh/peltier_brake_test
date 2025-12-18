#include <M5Unified.h>

// --- ピン定義 (Port A: GPIO 32, 33) ---
const int PIN_COOL = 32;  // Cool用 (PWM0)
const int PIN_HEAT = 33;  // Heat用 (PWM1)

// --- パラメータ設定 ---
// START設定（調整可能）
int startPower = 100;             // 0-255 (調整可能)
float startTimeSec = 1.0;         // 秒 (0.1秒刻みで調整可能)

// HEAT設定（固定）
int heatPower = 40;               // 0-255 (固定)
unsigned long heatTimeMs = 3000;  // 3秒 (固定)

// BRAKE設定（固定）
int brakePower = 240;             // PWM240で冷却 (固定)
unsigned long brakeTimeMs = 900;  // 0.9秒 (固定)

// COOL設定（固定）
int coolPower = 240;              // PWM240 (固定)
unsigned long coolTimeMs = 3000;  // 3秒 (固定)

// --- 内部変数 ---
enum State {
  STATE_IDLE,
  STATE_START,
  STATE_HEAT,
  STATE_BRAKE,
  STATE_COOL
};

State currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
unsigned long startDurationMs = 0;

// PWMチャンネル
const int PWM_CH_COOL = 0;
const int PWM_CH_HEAT = 1;
const int PWM_FREQ = 1000;
const int PWM_RES = 8;

// 設定画面の変数
bool settingComplete = false;

// 関数プロトタイプ
void setPeltier(int powerCool, int powerHeat);
void startPhaseStart();
void startPhaseHeat();
void startPhaseBrake();
void startPhaseCool();
void stopPeltier();
void drawSettingUI();
void drawRunningUI();
void handleSettingTouch();

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  Serial.begin(115200);
  M5.Display.setBrightness(255);
  
  // PWM設定
  ledcSetup(PWM_CH_COOL, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_HEAT, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_COOL, PWM_CH_COOL);
  ledcAttachPin(PIN_HEAT, PWM_CH_HEAT);

  // START時間をミリ秒に変換
  startDurationMs = (unsigned long)(startTimeSec * 1000);

  Serial.println("========================================");
  Serial.println("Peltier 4-Phase Test");
  Serial.println("START -> HEAT -> BRAKE -> COOL");
  Serial.println("Port A (GPIO 32, 33)");
  Serial.println("========================================");
  
  drawSettingUI();
}

void loop() {
  M5.update();
  
  if (!settingComplete) {
    handleSettingTouch();
    return;
  }

  // 状態マシン
  unsigned long elapsed = millis() - stateStartTime;

  switch (currentState) {
    case STATE_START:
      if (elapsed >= startDurationMs) {
        startPhaseHeat(); // START終了 -> HEATへ
      }
      break;

    case STATE_HEAT:
      if (elapsed >= heatTimeMs) {
        startPhaseBrake(); // HEAT終了 -> BRAKEへ
      }
      break;

    case STATE_BRAKE:
      if (elapsed >= brakeTimeMs) {
        startPhaseCool(); // BRAKE終了 -> COOLへ
      }
      break;

    case STATE_COOL:
      if (elapsed >= coolTimeMs) {
        stopPeltier(); // COOL終了 -> 停止
        currentState = STATE_IDLE;
        Serial.println("Cycle Complete.");
        drawRunningUI();
      }
      break;

    case STATE_IDLE:
      auto t = M5.Touch.getDetail();
      if (t.wasPressed()) {
        int x = t.x;
        int y = t.y;
        
        // STARTボタン
        if (y >= 135 && y <= 175 && x >= 70 && x <= 250) {
          startPhaseStart();
          drawRunningUI();
        }
        // BACKボタン (設定画面に戻る)
        else if (y >= 205 && y <= 235 && x >= 85 && x <= 235) {
          settingComplete = false;
          drawSettingUI();
        }
      }
      break;
  }
  
  delay(10);
}

// --- 制御関数 ---

void startPhaseStart() {
  Serial.println("Phase: START");
  Serial.printf("  PWM: Cool=0 Heat=%d\n", startPower);
  Serial.printf("  Duration: %.1f sec\n", startTimeSec);
  setPeltier(0, startPower);
  currentState = STATE_START;
  stateStartTime = millis();
}

void startPhaseHeat() {
  Serial.println("Phase: HEAT");
  Serial.printf("  PWM: Cool=0 Heat=%d\n", heatPower);
  Serial.printf("  Duration: %lu ms\n", heatTimeMs);
  setPeltier(0, heatPower);
  currentState = STATE_HEAT;
  stateStartTime = millis();
  drawRunningUI();
}

void startPhaseBrake() {
  Serial.println("Phase: BRAKE (Cooling)");
  Serial.printf("  PWM: Cool=%d Heat=0\n", brakePower);
  Serial.printf("  Duration: %lu ms\n", brakeTimeMs);
  setPeltier(brakePower, 0);
  currentState = STATE_BRAKE;
  stateStartTime = millis();
  drawRunningUI();
}

void startPhaseCool() {
  Serial.println("Phase: COOL");
  Serial.printf("  PWM: Cool=%d Heat=0\n", coolPower);
  Serial.printf("  Duration: %lu ms\n", coolTimeMs);
  setPeltier(coolPower, 0);
  currentState = STATE_COOL;
  stateStartTime = millis();
  drawRunningUI();
}

void stopPeltier() {
  setPeltier(0, 0);
  Serial.println("State: STOP");
  Serial.println("  PWM: Cool=0 Heat=0");
}

void setPeltier(int powerCool, int powerHeat) {
  ledcWrite(PWM_CH_COOL, powerCool);
  ledcWrite(PWM_CH_HEAT, powerHeat);
}

// --- UI関数 ---

void drawSettingUI() {
  M5.Display.clear(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(50, 5);
  M5.Display.println("4-Phase Test");
  
  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 30);
  M5.Display.println("START(adj) > HEAT > BRAKE > COOL");

  // START Power調整
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 50);
  M5.Display.println("START Power:");

  M5.Display.fillRect(10, 75, 40, 30, RED);
  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(20, 80);
  M5.Display.println("-");

  M5.Display.fillRect(60, 75, 40, 30, GREEN);
  M5.Display.setCursor(70, 80);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 78);
  M5.Display.printf("%3d", startPower);

  // START Time調整
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 115);
  M5.Display.println("START Time(s):");

  M5.Display.fillRect(10, 140, 40, 30, RED);
  M5.Display.setCursor(20, 145);
  M5.Display.println("-");

  M5.Display.fillRect(60, 140, 40, 30, GREEN);
  M5.Display.setCursor(70, 145);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 143);
  M5.Display.printf("%.1f", startTimeSec);

  // 固定パラメータ表示
  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 180);
  M5.Display.println("HEAT:40,3s BRAKE:cool240,0.9s");
  M5.Display.setCursor(10, 195);
  M5.Display.println("COOL:240,3s");

  // DONEボタン
  M5.Display.fillRect(85, 210, 150, 25, YELLOW);
  M5.Display.setTextColor(BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(120, 215);
  M5.Display.println("DONE");
}

void handleSettingTouch() {
  auto t = M5.Touch.getDetail();
  if (!t.wasPressed()) return;

  int x = t.x;
  int y = t.y;
  bool redraw = false;

  // START Power調整
  if (y >= 75 && y <= 105) {
    if (x >= 10 && x <= 50) {
      startPower -= 10;
      if (startPower < 0) startPower = 0;
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      startPower += 10;
      if (startPower > 255) startPower = 255;
      redraw = true;
    }
  }

  // START Time調整 (0.1秒刻み)
  if (y >= 140 && y <= 170) {
    if (x >= 10 && x <= 50) {
      startTimeSec -= 0.1;
      if (startTimeSec < 0) startTimeSec = 0;
      startDurationMs = (unsigned long)(startTimeSec * 1000);
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      startTimeSec += 0.1;
      startDurationMs = (unsigned long)(startTimeSec * 1000);
      redraw = true;
    }
  }

  // DONEボタン
  if (y >= 210 && y <= 235 && x >= 85 && x <= 235) {
    settingComplete = true;
    drawRunningUI();
    return;
  }

  if (redraw) drawSettingUI();
}

void drawRunningUI() {
  M5.Display.clear(BLACK);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 5);
  M5.Display.println("4-Phase Cycle");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(5, 30);
  M5.Display.printf("START:heat%d,%.1fs", startPower, startTimeSec);
  M5.Display.setCursor(5, 43);
  M5.Display.printf("HEAT:heat40,3s");
  M5.Display.setCursor(5, 56);
  M5.Display.printf("BRAKE:cool240,0.9s");
  M5.Display.setCursor(5, 69);
  M5.Display.printf("COOL:cool240,3s");

  const char* stateText = "IDLE";
  uint16_t stateColor = WHITE;
  
  switch (currentState) {
    case STATE_START:
      stateText = "START";
      stateColor = ORANGE;
      break;
    case STATE_HEAT:
      stateText = "HEAT";
      stateColor = RED;
      break;
    case STATE_BRAKE:
      stateText = "BRAKE";
      stateColor = YELLOW;
      break;
    case STATE_COOL:
      stateText = "COOL";
      stateColor = CYAN;
      break;
    case STATE_IDLE:
      stateText = "IDLE";
      stateColor = WHITE;
      break;
  }

  M5.Display.setTextSize(4);
  M5.Display.setTextColor(stateColor);
  M5.Display.setCursor(50, 90);
  M5.Display.println(stateText);

  if (currentState == STATE_IDLE) {
    M5.Display.fillRect(70, 140, 180, 35, GREEN);
    M5.Display.setTextColor(BLACK);
    M5.Display.setTextSize(3);
    M5.Display.setCursor(100, 148);
    M5.Display.println("START");
    
    M5.Display.fillRect(85, 205, 150, 30, ORANGE);
    M5.Display.setTextColor(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(110, 212);
    M5.Display.println("BACK");
  }
}
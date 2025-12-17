#include <M5Unified.h>

// --- ピン定義 (Port A: GPIO 32, 33) ---
const int PIN_COOL = 32;  // Cool用 (PWM0)
const int PIN_HEAT = 33;  // Heat用 (PWM1)

// --- パラメータ設定 ---
// 冷却設定
int coolPower = 240;              // 0-255
unsigned long coolTimeMs = 3000;  // 冷却時間 (ms) = 3秒

// ★ブレーキ設定 (Cool -> Brake で終了)
int brakePower = 0;               // ブレーキ時の出力 (0=OFF, 両方ONで電気ブレーキ)
float brakeTimeSec = 0.5;         // ブレーキ時間 (秒) 0.05秒刻みで設定可能

// --- 内部変数 ---
enum State {
  STATE_IDLE,
  STATE_COOL,
  STATE_BRAKE
};

State currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
unsigned long brakeDurationMs = 0;

// PWMチャンネル
const int PWM_CH_COOL = 0;
const int PWM_CH_HEAT = 1;
const int PWM_FREQ = 1000;
const int PWM_RES = 8;

// 設定画面の変数
bool settingComplete = false;

// 関数プロトタイプ
void setPeltier(int powerCool, int powerHeat);
void startCool();
void startBrake();
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

  // ブレーキ時間をミリ秒に変換
  brakeDurationMs = (unsigned long)(brakeTimeSec * 1000);

  Serial.println("========================================");
  Serial.println("Peltier Brake System (Cool only)");
  Serial.println("Port A (GPIO 32, 33)");
  Serial.println("Cool: PWM240, 3sec");
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
    case STATE_COOL:
      if (elapsed >= coolTimeMs) {
        startBrake(); // Cool終了 -> Brakeへ
      }
      break;

    case STATE_BRAKE:
      if (elapsed >= brakeDurationMs) {
        stopPeltier(); // Brake終了 -> 停止
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
        
        // STARTボタン (70, 125, 180, 40)
        if (y >= 125 && y <= 165 && x >= 70 && x <= 250) {
          startCool();
          drawRunningUI();
        }
        // BACKボタン (設定画面に戻る)
        else if (y >= 200 && y <= 235 && x >= 85 && x <= 235) {
          settingComplete = false;
          drawSettingUI();
        }
      }
      break;
  }
  
  delay(10);
}

// --- 制御関数 ---

void startCool() {
  Serial.println("State: COOL");
  Serial.printf("  PWM: Cool=%d Heat=0\n", coolPower);
  Serial.printf("  Duration: %lu ms\n", coolTimeMs);
  setPeltier(coolPower, 0);
  currentState = STATE_COOL;
  stateStartTime = millis();
}

void startBrake() {
  Serial.printf("State: BRAKE (Time: %.2fs, Power: %d)\n", brakeTimeSec, brakePower);
  
  // ブレーキ実装
  // brakePower=0: 両方OFF (フリーラン)
  // brakePower>0: 両方ON (電気ブレーキ、逆起電力を素早く消費)
  if (brakePower == 0) {
    setPeltier(0, 0);
    Serial.println("  PWM: Cool=0 Heat=0 (Free Run)");
  } else {
    setPeltier(brakePower, brakePower);
    Serial.printf("  PWM: Cool=%d Heat=%d (Active Brake)\n", brakePower, brakePower);
  }
  
  currentState = STATE_BRAKE;
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
  M5.Display.setCursor(40, 10);
  M5.Display.println("Brake Settings");
  
  M5.Display.setTextSize(1);
  M5.Display.setCursor(20, 35);
  M5.Display.println("Port A (GPIO 32=Cool, 33=Heat)");
  M5.Display.setCursor(20, 50);
  M5.Display.println("Cool: PWM240, 3sec");

  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 75);
  M5.Display.println("Brake Power:");

  // -/+ ボタン
  M5.Display.fillRect(10, 100, 40, 30, RED);
  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(20, 105);
  M5.Display.println("-");

  M5.Display.fillRect(60, 100, 40, 30, GREEN);
  M5.Display.setCursor(70, 105);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 103);
  M5.Display.printf("%3d", brakePower);

  // Brake Time
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 145);
  M5.Display.println("Brake Time(s):");

  M5.Display.fillRect(10, 170, 40, 30, RED);
  M5.Display.setCursor(20, 175);
  M5.Display.println("-");

  M5.Display.fillRect(60, 170, 40, 30, GREEN);
  M5.Display.setCursor(70, 175);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 173);
  M5.Display.printf("%.2f", brakeTimeSec);

  // DONEボタン
  M5.Display.fillRect(85, 210, 150, 30, YELLOW);
  M5.Display.setTextColor(BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(120, 217);
  M5.Display.println("DONE");
}

void handleSettingTouch() {
  auto t = M5.Touch.getDetail();
  if (!t.wasPressed()) return;

  int x = t.x;
  int y = t.y;
  bool redraw = false;

  // Brake Power調整
  if (y >= 100 && y <= 130) {
    if (x >= 10 && x <= 50) {
      brakePower -= 10;
      if (brakePower < 0) brakePower = 0;
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      brakePower += 10;
      if (brakePower > 255) brakePower = 255;
      redraw = true;
    }
  }

  // Brake Time調整 (0.05秒刻み)
  if (y >= 170 && y <= 200) {
    if (x >= 10 && x <= 50) {
      brakeTimeSec -= 0.05;
      if (brakeTimeSec < 0) brakeTimeSec = 0;
      brakeDurationMs = (unsigned long)(brakeTimeSec * 1000);
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      brakeTimeSec += 0.05;
      brakeDurationMs = (unsigned long)(brakeTimeSec * 1000);
      redraw = true;
    }
  }

  // DONEボタン
  if (y >= 210 && y <= 240 && x >= 85 && x <= 235) {
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
  M5.Display.setCursor(30, 10);
  M5.Display.println("Cool -> Brake");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 40);
  M5.Display.printf("Cool: PWM%d, 3sec", coolPower);
  M5.Display.setCursor(10, 55);
  M5.Display.printf("Brake: PWM%d, %.2fs", brakePower, brakeTimeSec);

  const char* stateText = "IDLE";
  uint16_t stateColor = WHITE;
  
  switch (currentState) {
    case STATE_COOL:
      stateText = "COOLING";
      stateColor = CYAN;
      break;
    case STATE_BRAKE:
      stateText = "BRAKE";
      stateColor = YELLOW;
      break;
    case STATE_IDLE:
      stateText = "IDLE";
      stateColor = WHITE;
      break;
  }

  M5.Display.setTextSize(4);
  M5.Display.setTextColor(stateColor);
  M5.Display.setCursor(60, 80);
  M5.Display.println(stateText);

  if (currentState == STATE_IDLE) {
    M5.Display.fillRect(70, 135, 180, 40, GREEN);
    M5.Display.setTextColor(BLACK);
    M5.Display.setTextSize(3);
    M5.Display.setCursor(100, 145);
    M5.Display.println("START");
    
    M5.Display.fillRect(85, 205, 150, 30, ORANGE);
    M5.Display.setTextColor(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(110, 212);
    M5.Display.println("BACK");
  }
}
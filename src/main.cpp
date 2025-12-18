#include <M5Unified.h>

// --- ピン定義 (Port A: GPIO 32, 33) ---
const int PIN_COOL = 32;  // Cool用 (PWM0)
const int PIN_HEAT = 33;  // Heat用 (PWM1)

// --- 温刺激パラメータ ---
// heat_start (固定)
int heatStartPower = 240;          // PWM240
unsigned long heatStartTimeMs = 1000;  // 1秒

// heat (固定)
int heatPower = 40;                // PWM40
unsigned long heatTimeMs = 3000;   // 3秒

// heat_end (時間のみ調整可能)
int heatEndPower = 240;            // PWM240で冷却
float heatEndTimeSec = 1.0;        // 秒 (0.1秒刻みで調整可能)

// --- 冷刺激パラメータ ---
// cool_start (固定)
int coolStartPower = 240;          // PWM240
unsigned long coolStartTimeMs = 1000;  // 1秒

// cool (固定)
int coolPower = 240;               // PWM240
unsigned long coolTimeMs = 3000;   // 3秒

// cool_end (時間のみ調整可能)
int coolEndPower = 0;              // PWM0 (停止)
float coolEndTimeSec = 1.0;        // 秒 (0.1秒刻みで調整可能)

// --- 内部変数 ---
enum State {
  STATE_IDLE,
  STATE_HEAT_START,
  STATE_HEAT,
  STATE_HEAT_END,
  STATE_COOL_START,
  STATE_COOL,
  STATE_COOL_END
};

State currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
unsigned long heatEndDurationMs = 0;
unsigned long coolEndDurationMs = 0;

// PWMチャンネル
const int PWM_CH_COOL = 0;
const int PWM_CH_HEAT = 1;
const int PWM_FREQ = 1000;
const int PWM_RES = 8;

// 設定画面の変数
bool settingComplete = false;

// 関数プロトタイプ
void setPeltier(int powerCool, int powerHeat);
void startHeatStimulus();
void startCoolStimulus();
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

  // 終了時間をミリ秒に変換
  heatEndDurationMs = (unsigned long)(heatEndTimeSec * 1000);
  coolEndDurationMs = (unsigned long)(coolEndTimeSec * 1000);

  Serial.println("========================================");
  Serial.println("Peltier Thermal Stimulus Test");
  Serial.println("Port A (GPIO 32=Cool, 33=Heat)");
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
    case STATE_HEAT_START:
      if (elapsed >= heatStartTimeMs) {
        Serial.println("Phase: HEAT");
        setPeltier(0, heatPower);
        currentState = STATE_HEAT;
        stateStartTime = millis();
        drawRunningUI();
      }
      break;

    case STATE_HEAT:
      if (elapsed >= heatTimeMs) {
        Serial.println("Phase: HEAT_END (Cooling)");
        setPeltier(heatEndPower, 0);
        currentState = STATE_HEAT_END;
        stateStartTime = millis();
        drawRunningUI();
      }
      break;

    case STATE_HEAT_END:
      if (elapsed >= heatEndDurationMs) {
        stopPeltier();
        currentState = STATE_IDLE;
        Serial.println("Heat Stimulus Complete.\n");
        drawRunningUI();
      }
      break;

    case STATE_COOL_START:
      if (elapsed >= coolStartTimeMs) {
        Serial.println("Phase: COOL");
        setPeltier(coolPower, 0);
        currentState = STATE_COOL;
        stateStartTime = millis();
        drawRunningUI();
      }
      break;

    case STATE_COOL:
      if (elapsed >= coolTimeMs) {
        Serial.println("Phase: COOL_END");
        setPeltier(coolEndPower, 0);
        currentState = STATE_COOL_END;
        stateStartTime = millis();
        drawRunningUI();
      }
      break;

    case STATE_COOL_END:
      if (elapsed >= coolEndDurationMs) {
        stopPeltier();
        currentState = STATE_IDLE;
        Serial.println("Cool Stimulus Complete.\n");
        drawRunningUI();
      }
      break;

    case STATE_IDLE:
      auto t = M5.Touch.getDetail();
      if (t.wasPressed()) {
        int x = t.x;
        int y = t.y;
        
        // HEAT STIMボタン
        if (y >= 120 && y <= 155 && x >= 20 && x <= 150) {
          startHeatStimulus();
          drawRunningUI();
        }
        // COOL STIMボタン
        else if (y >= 120 && y <= 155 && x >= 170 && x <= 300) {
          startCoolStimulus();
          drawRunningUI();
        }
        // BACKボタン
        else if (y >= 200 && y <= 230 && x >= 85 && x <= 235) {
          settingComplete = false;
          drawSettingUI();
        }
      }
      break;
  }
  
  delay(10);
}

// --- 制御関数 ---

void startHeatStimulus() {
  Serial.println("\n=== HEAT STIMULUS START ===");
  Serial.println("Phase: HEAT_START");
  setPeltier(0, heatStartPower);
  currentState = STATE_HEAT_START;
  stateStartTime = millis();
}

void startCoolStimulus() {
  Serial.println("\n=== COOL STIMULUS START ===");
  Serial.println("Phase: COOL_START");
  setPeltier(coolStartPower, 0);
  currentState = STATE_COOL_START;
  stateStartTime = millis();
}

void stopPeltier() {
  setPeltier(0, 0);
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
  M5.Display.setCursor(30, 5);
  M5.Display.println("Thermal Stim Test");
  
  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 30);
  M5.Display.println("Port A: Heat & Cool stimulus");

  // Heat End Time調整
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 55);
  M5.Display.println("Heat End(s):");

  M5.Display.fillRect(10, 80, 40, 30, RED);
  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(20, 85);
  M5.Display.println("-");

  M5.Display.fillRect(60, 80, 40, 30, GREEN);
  M5.Display.setCursor(70, 85);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 83);
  M5.Display.printf("%.1f", heatEndTimeSec);

  // Cool End Time調整
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 120);
  M5.Display.println("Cool End(s):");

  M5.Display.fillRect(10, 145, 40, 30, RED);
  M5.Display.setCursor(20, 150);
  M5.Display.println("-");

  M5.Display.fillRect(60, 145, 40, 30, GREEN);
  M5.Display.setCursor(70, 150);
  M5.Display.println("+");

  M5.Display.setTextSize(3);
  M5.Display.setCursor(110, 148);
  M5.Display.printf("%.1f", coolEndTimeSec);

  // 固定パラメータ表示
  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 185);
  M5.Display.println("Heat: 240,1s > 40,3s > 240cool");
  M5.Display.setCursor(10, 198);
  M5.Display.println("Cool: 240,1s > 240,3s > stop");

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

  // Heat End Time調整 (0.1秒刻み)
  if (y >= 80 && y <= 110) {
    if (x >= 10 && x <= 50) {
      heatEndTimeSec -= 0.1;
      if (heatEndTimeSec < 0) heatEndTimeSec = 0;
      heatEndDurationMs = (unsigned long)(heatEndTimeSec * 1000);
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      heatEndTimeSec += 0.1;
      heatEndDurationMs = (unsigned long)(heatEndTimeSec * 1000);
      redraw = true;
    }
  }

  // Cool End Time調整 (0.1秒刻み)
  if (y >= 145 && y <= 175) {
    if (x >= 10 && x <= 50) {
      coolEndTimeSec -= 0.1;
      if (coolEndTimeSec < 0) coolEndTimeSec = 0;
      coolEndDurationMs = (unsigned long)(coolEndTimeSec * 1000);
      redraw = true;
    } else if (x >= 60 && x <= 100) {
      coolEndTimeSec += 0.1;
      coolEndDurationMs = (unsigned long)(coolEndTimeSec * 1000);
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
  M5.Display.setCursor(50, 5);
  M5.Display.println("Port A Test");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(5, 35);
  M5.Display.printf("Heat: 240,1s>40,3s>cool240,%.1fs", heatEndTimeSec);
  M5.Display.setCursor(5, 50);
  M5.Display.printf("Cool: 240,1s>240,3s>stop%.1fs", coolEndTimeSec);

  const char* stateText = "IDLE";
  uint16_t stateColor = WHITE;
  
  switch (currentState) {
    case STATE_HEAT_START:
      stateText = "HEAT_START";
      stateColor = ORANGE;
      break;
    case STATE_HEAT:
      stateText = "HEAT";
      stateColor = RED;
      break;
    case STATE_HEAT_END:
      stateText = "HEAT_END";
      stateColor = YELLOW;
      break;
    case STATE_COOL_START:
      stateText = "COOL_START";
      stateColor = CYAN;
      break;
    case STATE_COOL:
      stateText = "COOL";
      stateColor = BLUE;
      break;
    case STATE_COOL_END:
      stateText = "COOL_END";
      stateColor = PURPLE;
      break;
    case STATE_IDLE:
      stateText = "IDLE";
      stateColor = WHITE;
      break;
  }

  M5.Display.setTextSize(3);
  M5.Display.setTextColor(stateColor);
  M5.Display.setCursor(20, 75);
  M5.Display.println(stateText);

  if (currentState == STATE_IDLE) {
    // HEAT STIMボタン
    M5.Display.fillRect(20, 120, 130, 35, RED);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(30, 130);
    M5.Display.println("HEAT");
    
    // COOL STIMボタン
    M5.Display.fillRect(170, 120, 130, 35, CYAN);
    M5.Display.setTextColor(BLACK);
    M5.Display.setCursor(180, 130);
    M5.Display.println("COOL");
    
    // BACKボタン
    M5.Display.fillRect(85, 200, 150, 30, ORANGE);
    M5.Display.setTextColor(BLACK);
    M5.Display.setCursor(110, 207);
    M5.Display.println("BACK");
  }
}
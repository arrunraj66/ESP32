#pragma once
#include <Arduino.h>

// =====================================================
//  MODULE SWITCHES  (1 = enabled, 0 = disabled)
//  Turn a module off here and its code and library are not compiled.
// =====================================================
#define USE_COMMS     1   // serial command/status link (later: WiFi / ZeroMQ bridge)
#define USE_SERVOS    1
#define USE_DISPLAY   0
#define USE_AUDIO     0
#define USE_DISTANCE  0   // VL53L1X
#define USE_BEHAVIOR  0   // idle look-around + reactions

// =====================================================
//  SERVOS  (power them from the buck converter, NOT the ESP32 5V pin)
// =====================================================
#define NUM_SERVOS 4
enum ServoId { SERVO_PAN = 0, SERVO_TILT, SERVO_LEFT_ARM, SERVO_RIGHT_ARM };
static const int SERVO_PINS[NUM_SERVOS]   = { 1,  2, 42, 41 };
static const int SERVO_MIN[NUM_SERVOS]    = { 20, 60, 10, 10 };   // software limits (deg)
static const int SERVO_MAX[NUM_SERVOS]    = {160,120,170,170};
static const int SERVO_CENTER[NUM_SERVOS] = { 90, 90, 90, 90 };

// =====================================================
//  TFT 2.8" ILI9341 (SPI)   LED/BL pin -> 3.3V
// =====================================================
#define TFT_SCK  12
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_CS   10
#define TFT_DC   14
#define TFT_RST  21

// =====================================================
//  AUDIO (I2S)  INMP441 mic + MAX98357A amp
// =====================================================
#define AUDIO_SAMPLE_RATE 16000
#define MIC_SCK 4
#define MIC_WS  5
#define MIC_SD  6
#define SPK_BCLK 7
#define SPK_LRC  15
#define SPK_DIN  16
#define BEEP_AMPLITUDE 3000        // keep low (max 32767)
#define AUDIO_LOUD_THRESHOLD 30000 // peak level that counts as a clap/loud sound

// =====================================================
//  I2C (VL53L1X distance sensor)
// =====================================================
#define I2C_SDA 8
#define I2C_SCL 9

// =====================================================
//  COMMS: which bus topics are printed out on Serial
// =====================================================
static const char* const OUTBOUND_TOPICS[] = { "distance", "audio/loud" };
#define OUTBOUND_COUNT (sizeof(OUTBOUND_TOPICS) / sizeof(OUTBOUND_TOPICS[0]))

// =====================================================
//  micro-ZMQ (WiFi link to the agent -> your ZeroMQ network)
// =====================================================
#define USE_ZMQ 1
#define WIFI_SSID       "your_wifi_name"
#define WIFI_PASS       "your_wifi_password"
#define ZMQ_AGENT_HOST  "192.168.1.50"     // PC (or cloud server) running agent.py
#define ZMQ_AGENT_PORT  7400
#define ZMQ_NODE_NAME   "robot-body"
#define ZMQ_TOKEN       "change-me"        // must match agent --token
// bus topics that are forwarded to ZeroMQ as "robot/out/<topic>"
static const char* const ZMQ_OUTBOUND_TOPICS[] = { "distance", "audio/loud", "audio/level", "emotion" };
#define ZMQ_OUTBOUND_COUNT (sizeof(ZMQ_OUTBOUND_TOPICS) / sizeof(ZMQ_OUTBOUND_TOPICS[0]))

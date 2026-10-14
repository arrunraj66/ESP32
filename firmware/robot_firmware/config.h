#pragma once
#include <Arduino.h>

// =====================================================
//  MODULE SWITCHES  (1 = enabled, 0 = disabled)
//  Turn a module off here and its code and library are not compiled.
// =====================================================
#define USE_COMMS     1   // serial command/status link (later: WiFi / ZeroMQ bridge)
#define USE_SERVOS    1   // positional servos (camera/laser scanner)
#define USE_DRIVE     1   // 4 continuous-rotation servo wheels (skid steer)
#define USE_DISPLAY   0
#define USE_AUDIO     0
#define USE_DISTANCE  0   // VL53L1X
#define USE_BEHAVIOR  0   // idle look-around + reactions

// =====================================================
//  SERVOS  (power them from the buck converter, NOT the ESP32 5V pin)
// =====================================================
#define NUM_SERVOS 1
enum ServoId { SERVO_SCANNER = 0 };
static const int SERVO_PINS[NUM_SERVOS]   = { 40 };
static const int SERVO_MIN[NUM_SERVOS]    = { 0 };     // software limits (deg)
static const int SERVO_MAX[NUM_SERVOS]    = { 180 };
static const int SERVO_CENTER[NUM_SERVOS] = { 90 };

// =====================================================
//  DRIVE: 4 SG90 servos modified for continuous rotation (skid steer)
//  Pulse 1500 us = stop, shorter = one way, longer = the other way.
//  Power from the buck converter, NOT the ESP32 pin. Keep all grounds joined.
// =====================================================
#define NUM_WHEELS 4
enum WheelId { WHEEL_FL = 0, WHEEL_RL, WHEEL_FR, WHEEL_RR };   // front/rear, left/right
static const int  WHEEL_PINS[NUM_WHEELS]   = { 1, 2, 42, 41 };
static const bool WHEEL_LEFT[NUM_WHEELS]   = { true, true, false, false };
// Mirrored servos need opposite pulses to roll the same way. Fix with "drive/raw" test, then edit here.
static const bool WHEEL_INVERT[NUM_WHEELS] = { false, false, true, true };
static int WHEEL_STOP_US[NUM_WHEELS]       = { 1500, 1500, 1500, 1500 };   // trim if a wheel creeps at stop
#define DRIVE_RANGE_US    400     // full speed = stop +/- this many microseconds
#define DRIVE_MAX_SCALE   0.5f    // start slow (0.0-1.0), raise once it works
#define DRIVE_TIMEOUT_MS  500     // stop if no drive/cmd arrives for this long
#define DRIVE_RAMP_PER_S  3.0f    // max speed change per second (1.0 = full), avoids current spikes

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
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy secrets.example.h to secrets.h and fill in your values"
#endif
#define ZMQ_AGENT_PORT  7400
#define ZMQ_NODE_NAME   "robot-body"
// bus topics that are forwarded to ZeroMQ as "robot/out/<topic>"
static const char* const ZMQ_OUTBOUND_TOPICS[] = { "distance", "audio/loud", "audio/level", "emotion" };
#define ZMQ_OUTBOUND_COUNT (sizeof(ZMQ_OUTBOUND_TOPICS) / sizeof(ZMQ_OUTBOUND_TOPICS[0]))

#include "mpu6050.h"
#include "config.h"
#include <Wire.h>
#include <math.h>

// Direct-register I2C driver (no Adafruit_MPU6050). The Adafruit lib's
// begin() does a strict WHO_AM_I check that some MPU6050/GY-521 clone
// boards fail even though the sensor works fine, so we talk to the
// registers ourselves instead.

static const uint8_t MPU_ADDR = 0x68;       // AD0 low (default on most GY-521 boards)
static const uint8_t REG_WHO_AM_I = 0x75;
static const uint8_t REG_PWR_MGMT_1 = 0x6B;
static const uint8_t REG_GYRO_CONFIG = 0x1B;
static const uint8_t REG_ACCEL_CONFIG = 0x1C;
static const uint8_t REG_CONFIG = 0x1A;
static const uint8_t REG_ACCEL_XOUT_H = 0x3B;

static const float ACCEL_LSB_PER_G = 4096.0f;   // +-8g range
static const float GYRO_LSB_PER_DPS = 65.5f;    // +-500 deg/s range

static ImuReading latest;
static uint32_t lastReadMs = 0;
static bool available = false;

static bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool readRegs(uint8_t reg, uint8_t *buf, size_t len) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)len) != len) return false;
  for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

bool mpuInit() {
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  Wire.setClock(400000);

  uint8_t whoAmI = 0;
  if (!readRegs(REG_WHO_AM_I, &whoAmI, 1)) return false;
  // Accept known MPU-60x0/6500/9250-family IDs; clone boards vary slightly.
  if (whoAmI != 0x68 && whoAmI != 0x69 && whoAmI != 0x70 && whoAmI != 0x72 && whoAmI != 0x73) {
    return false;
  }

  if (!writeReg(REG_PWR_MGMT_1, 0x00)) return false;   // wake from sleep, internal clock
  delay(10);
  writeReg(REG_CONFIG, 0x03);          // DLPF ~44Hz (close to prior 21Hz bandwidth)
  writeReg(REG_GYRO_CONFIG, 0x08);     // +-500 deg/s
  writeReg(REG_ACCEL_CONFIG, 0x10);    // +-8g

  available = true;
  return true;
}

void mpuPoll() {
  if (!available) return;
  uint32_t now = millis();
  if (now - lastReadMs < MPU_READ_INTERVAL_MS) return;
  lastReadMs = now;

  uint8_t raw[14];
  if (!readRegs(REG_ACCEL_XOUT_H, raw, sizeof(raw))) return;

  int16_t axRaw = (int16_t)((raw[0] << 8) | raw[1]);
  int16_t ayRaw = (int16_t)((raw[2] << 8) | raw[3]);
  int16_t azRaw = (int16_t)((raw[4] << 8) | raw[5]);
  // raw[6..7] is temperature, unused
  int16_t gxRaw = (int16_t)((raw[8] << 8) | raw[9]);
  int16_t gyRaw = (int16_t)((raw[10] << 8) | raw[11]);
  int16_t gzRaw = (int16_t)((raw[12] << 8) | raw[13]);

  float ax = axRaw / ACCEL_LSB_PER_G;
  float ay = ayRaw / ACCEL_LSB_PER_G;
  float az = azRaw / ACCEL_LSB_PER_G;
  latest.accelMagG = sqrtf(ax * ax + ay * ay + az * az);

  float gx = gxRaw / GYRO_LSB_PER_DPS;
  float gy = gyRaw / GYRO_LSB_PER_DPS;
  float gz = gzRaw / GYRO_LSB_PER_DPS;
  latest.gyroMagDps = sqrtf(gx * gx + gy * gy + gz * gz);
}

ImuReading mpuGetLatest() { return latest; }

bool mpuDetectCrash(const ImuReading &r) {
  // Sudden high-G deviation from resting 1g, OR abnormal rotation rate.
  float deviationG = fabsf(r.accelMagG - 1.0f);
  return deviationG >= IMPACT_G_THRESHOLD || r.gyroMagDps >= GYRO_DPS_THRESHOLD;
}

#include "mpu6050.h"
#include "config.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

static Adafruit_MPU6050 mpu;
static ImuReading latest;
static uint32_t lastReadMs = 0;

bool mpuInit() {
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  if (!mpu.begin()) return false;
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  return true;
}

void mpuPoll() {
  uint32_t now = millis();
  if (now - lastReadMs < MPU_READ_INTERVAL_MS) return;
  lastReadMs = now;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  const float G = 9.80665f;
  float ax = a.acceleration.x / G, ay = a.acceleration.y / G, az = a.acceleration.z / G;
  latest.accelMagG = sqrtf(ax * ax + ay * ay + az * az);

  float gx = g.gyro.x * 57.2958f, gy = g.gyro.y * 57.2958f, gz = g.gyro.z * 57.2958f; // rad/s -> deg/s
  latest.gyroMagDps = sqrtf(gx * gx + gy * gy + gz * gz);
}

ImuReading mpuGetLatest() { return latest; }

bool mpuDetectCrash(const ImuReading &r) {
  // Sudden high-G deviation from resting 1g, OR abnormal rotation rate.
  float deviationG = fabsf(r.accelMagG - 1.0f);
  return deviationG >= IMPACT_G_THRESHOLD || r.gyroMagDps >= GYRO_DPS_THRESHOLD;
}

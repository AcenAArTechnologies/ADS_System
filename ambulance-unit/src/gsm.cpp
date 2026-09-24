#include "gsm.h"
#include "config.h"
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

static HardwareSerial gsmSerial(2);
static TinyGsm modem(gsmSerial);

void gsmInit() {
  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);

#if defined(GSM_RESET_PIN)
  if (GSM_RESET_PIN >= 0) {
    pinMode(GSM_RESET_PIN, OUTPUT);
    digitalWrite(GSM_RESET_PIN, HIGH);
    delay(100);
    digitalWrite(GSM_RESET_PIN, LOW);
    delay(1000);
    digitalWrite(GSM_RESET_PIN, HIGH);
  }
#endif

  delay(3000);
  Serial.println("GSM: restarting modem...");
  if (!modem.restart()) {
    Serial.println("GSM: modem did not respond to restart (check wiring/power)");
  } else {
    Serial.print("GSM: modem info: ");
    Serial.println(modem.getModemInfo());
  }
}

bool gsmWaitForNetwork(uint32_t timeoutMs) {
  return modem.waitForNetwork(timeoutMs);
}

int gsmSignalPercent() {
  int csq = modem.getSignalQuality();
  if (csq == 99) return -1;
  return (csq * 100) / 31;
}

bool gsmSendSms(const char *number, const String &text) {
  return modem.sendSMS(number, text);
}

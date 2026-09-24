#include "telegram.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD_MMC.h>

static const char *TELEGRAM_HOST = "api.telegram.org";

bool telegramUploadVideo(const String &sdPath) {
  File f = SD_MMC.open(sdPath, FILE_READ);
  if (!f) return false;
  size_t fileSize = f.size();

  WiFiClientSecure client;
  client.setInsecure(); // no cert pinning for v1; acceptable for a project-scale deployment
  if (!client.connect(TELEGRAM_HOST, 443)) {
    f.close();
    return false;
  }

  String boundary = "----ADSBoundary7MA4YWxkTrZu0gW";
  String fieldChatId =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + String(TELEGRAM_CHAT_ID) + "\r\n";
  String fieldCaption =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
      "Accident clip from " DEVICE_ID "\r\n";
  String fileHeader =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"video\"; filename=\"clip.avi\"\r\n"
      "Content-Type: video/avi\r\n\r\n";
  String fileFooter = "\r\n--" + boundary + "--\r\n";

  size_t contentLength = fieldChatId.length() + fieldCaption.length() + fileHeader.length() + fileSize + fileFooter.length();

  String path = "/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendVideo";
  client.print("POST " + path + " HTTP/1.1\r\n");
  client.print("Host: " + String(TELEGRAM_HOST) + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
  client.print("Content-Length: " + String(contentLength) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  client.print(fieldChatId);
  client.print(fieldCaption);
  client.print(fileHeader);

  uint8_t buf[1024];
  size_t sent = 0;
  while (sent < fileSize) {
    size_t chunk = f.read(buf, sizeof(buf));
    if (chunk == 0) break;
    client.write(buf, chunk);
    sent += chunk;
  }
  client.print(fileFooter);
  f.close();

  // Read status line to confirm success.
  uint32_t start = millis();
  String statusLine;
  while (client.connected() && millis() - start < 15000) {
    if (client.available()) {
      statusLine = client.readStringUntil('\n');
      break;
    }
  }
  client.stop();

  bool ok = statusLine.indexOf("200") > 0;
  if (ok) SD_MMC.remove(sdPath);
  return ok;
}

void telegramRetryPending() {
  if (WiFi.status() != WL_CONNECTED) return;

  File dir = SD_MMC.open(PENDING_UPLOAD_DIR);
  if (!dir || !dir.isDirectory()) return;

  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String path = String(entry.name());
      if (!path.startsWith("/")) path = String(PENDING_UPLOAD_DIR) + "/" + path;
      entry.close();
      telegramUploadVideo(path);
    } else {
      entry.close();
    }
    entry = dir.openNextFile();
  }
  dir.close();
}

#include <Arduino.h>
#include <esp_now.h>
#include <pb_encode.h>
#include "CamFreenove.h"
#include "frame.pb.h"

#define CONVERT_TO_JPEG

CamFreenove Camera;

/// general buffer for msg sender
uint8_t send_buffer[256];

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  if (!Camera.begin()) {
    Serial.println("Camera Init Fail");
  }
  Camera.sensor->set_framesize(Camera.sensor, FRAMESIZE_QVGA); 

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
}

uint16_t frame = 0;

void printFPS(const char *msg) {
  static uint_least64_t timeStamp = 0;
  frame++;
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    Serial.printf("%s %2d FPS\r\n",msg, frame);
    frame = 0;
  } 
}

size_t encodeMsg(Frame msg) {
  pb_ostream_t stream = pb_ostream_from_buffer(send_buffer, sizeof(send_buffer));
  bool status = pb_encode(&stream, Frame_fields, &msg);
  size_t message_length = stream.bytes_written;
  if (!status) {
    printf("Encoding failed: %s\r\n", PB_GET_ERROR(&stream));
    return 0;
  }
  return message_length;
}

bool sendMessage(uint32_t msglen, const uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, mac, 6);
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_add_peer(&peerInfo);
  }
  esp_err_t result = esp_now_send(mac, send_buffer, msglen);

  if (result == ESP_OK) {
    // Serial.println("send msg success");
    return true;
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Unknown error");
  }
  return false;
}

void dispatchFrame(uint8_t *data, uint32_t data_len) {
    Frame msg = Frame_init_zero;
    msg.chunk_left = data_len;
    msg.chunk_max = data_len;
    msg.lenght = 220;
    uint8_t chunk [220];
    memcpy(&chunk, data, 220);
    msg.data.arg = &chunk;
    // msg.data.funcs.encode = Frame
}

void loop() {
  if (Camera.get()) {
#ifdef CONVERT_TO_JPEG
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 64, &out_jpg, &out_jpg_len);
    printFPS("JPG compression at");
    dispatchFrame(out_jpg, out_jpg_len);
    // Display.drawJpg(out_jpg, out_jpg_len, 0, 0, dw, dh);
    free(out_jpg);
#else
    printFPS("frame ready at");
    // Display.pushImage(0, 0, dw, dh, (uint16_t *)CoreS3.Camera.fb->buf);
#endif
    Camera.free();
  }
}

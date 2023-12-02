#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define AWS_IOT_PUBLISH_TOPIC "bay/file"

const char *url;
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 0;

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

//****Function Declarations****
String currentDateTime();
void publishMessage(String filename);
void clickImage();
bool uploadImageToS3(uint8_t *imageData, size_t imageLen);
void setup();
void loop();
//********************************

/**
 * Sets the Current Data Time to be used as the filename
 * @return current date and time as a string
 */
String currentDateTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "FAIL";
  }
  char time[8];
  strftime(time, 8, "%H_%M_%s:", &timeinfo);

  char date[9];
  strftime(date, 17, "%d_%B_%Y", &timeinfo);
  String dateTime = String(date) + "_" + String(time);
  return dateTime;
}

/**
 * Publishes a message to the AWS IoT Core using MQTT Protocol
 * @param filename The filename of the image to be published
 * @param bayNum The bay number of the image to be published
 */
void publishMessage(String filename, String bayNum)
{
  StaticJsonDocument<200> doc;
  doc["bayNum"] = bayNum;
  doc["filename"] = "/demo/" + filename;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
/**
 * Function to click an image and then upload it to AWS S3 Cloud Storage
 */
void clickImage()
{
  // Capture an image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Failed to capture image");
    return;
  }
  // Upload image to AWS S3
  if (fb->len >= 90 * 1024)
  {
    if (uploadImageToS3(fb->buf, fb->len))
    {
      Serial.println("Image uploaded to AWS S3 successfully");
    }
    else
    {
      Serial.println("Failed to upload image to AWS S3");
    }

    delay(10000); // Delay for 5 seconds before capturing the next image
  }

  // Free memory used by the captured image
  esp_camera_fb_return(fb);
}
/**
 * Function to upload an image to AWS S3 Cloud Storage
 * Obtains a presigned URL from AWS Lambda using a HTTP Get Request
 * Uploads the image to the presigned URL using a HTTP Put Request
 */
bool uploadImageToS3(uint8_t *imageData, size_t imageLen)
{
  WiFiClient client;
  HTTPClient http;
  String filename = "bay2/" + currentDateTime();
  String api_link = getPreSignedURL + filename;
  http.begin(api_link); // Replace with your API endpoint
  int httpCode = http.GET();

  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();

      // Parse JSON
      DynamicJsonDocument jsonDoc(1024);
      deserializeJson(jsonDoc, payload);

      // Access JSON data
      url = jsonDoc["url"]; // Replace with the actual key in your JSON response
      Serial.print("Value: ");
      Serial.println(url);
    }
  }
  else
  {
    Serial.printf("HTTP GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  http.begin(url);
  httpCode = http.sendRequest("PUT", imageData, imageLen);
  Serial.println(httpCode);

  // Check if the image was uploaded successfully
  bool success = (httpCode == HTTP_CODE_OK);

  // Cleanup resources
  http.end();
  client.stop();
  if (success)
  {
    publishMessage(filename, "2");
  }
  return success;
}

void setup()
{
  Serial.begin(115200);
  Serial.println(WIFI_SSID);
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  currentDateTime();
  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera initialization failed with error 0x%x", err);
    return;
  }

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  // client.onMessage(messageHandler);
  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  // client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void loop()
{
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    while (!client.connect(THINGNAME))
    {
      Serial.print(".");
      delay(100);
    }
  }
  clickImage();
  client.loop();
  delay(1000);
}

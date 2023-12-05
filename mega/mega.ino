#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson (use v6.xx)
#include <MFRC522.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_DIO 53
#define RESET_DIO 28
#define SDA_DIO_2 46
#define RESET_DIO_2 40
#define ENTRY_RED 43
#define ENTRY_GREEN 41
#define SERVO_PIN 4
#define BUZZER_PIN 34
#define LIGHT_SENSOR 7
#define MOTION_SENSOR 5
#define TRIG_PIN 6
#define ECHO_PIN 7

bool validCar = false;

MFRC522 RC522_EntryGate(SDA_DIO, RESET_DIO);
MFRC522 RC522_Bay(SDA_DIO_2, RESET_DIO_2);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo servo;

// **************
double calcDist();
String prepareDataForEntryGate(String rfidTag, String topic);
String prepareDataForBay(String rfidTag, String topic, String bayNum, String parked);
String prepareDataForAdmin(String message, String topic);
String sendDataToWiFiBoard(String command, const int timeout);
void displayDisplayCenter(String text);
void setup();
void loop();
// **************

/**
 * Calculates the distance of an object from the ultrasonic sensor
 */
double calcDist()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(10000);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10000);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance_cm = (duration * 0.034) / 2;

  Serial.print("Distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  return distance_cm;
}

/**
 * Prepares the data to be sent to the WiFi board to validate whether the RFID Tag is valid or not
 * @param rfidTag The RFID Tag to be validated
 * @param topic The topic to be sent to the WiFi board
 * @return The JSON string to be sent to the WiFi board
 */
String prepareDataForEntryGate(String rfidTag, String topic)
{
  StaticJsonDocument<200> doc;
  doc["topic"] = topic;
  doc["rfidTag"] = rfidTag;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  return jsonBuffer;
}
/**
 * Prepares the data to be sent to the WiFi board to with Bay Number where the car is parked
 * @param rfidTag The RFID Tag parked in the bay
 * @param topic The topic to be sent to the WiFi board
 * @param bayNum The bay number where the car is parked
 * @param parked Whether the car is parked or not
 * @return The JSON string to be sent to the WiFi board
 */
String prepareDataForBay(String rfidTag, String topic, String bayNum, String parked)
{
  StaticJsonDocument<200> doc;
  doc["topic"] = topic;
  doc["bayNum"] = bayNum;
  doc["rfidTag"] = rfidTag;
  doc["parked"] = parked;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  return jsonBuffer;
}

/**
 * Prepares the data to be sent to the WiFi board when general security measures such as low light and motion detected
 * @param message The message to be sent to the WiFi board
 * @param topic The topic to be sent to the WiFi board
 * @return The JSON string to be sent to the WiFi board
 */
String prepareDataForAdmin(String message, String topic)
{
  StaticJsonDocument<200> doc;
  doc["topic"] = topic;
  doc["message"] = message;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  return jsonBuffer;
}

/**
 * Sends the data to the WiFi board and waits for the response
 * @param command The command to be sent to the WiFi board in JSON format
 * @param timeout The timeout to wait for the response
 * @return The response from the WiFi board
 */

String sendDataToWiFiBoard(String command, const int timeout)
{
  String response = "";

  Serial1.print(command); // Send the JSON to the WiFi board

  long int time = millis();

  while ((time + timeout) > millis())
  {
    while (Serial1.available())
    {
      char c = Serial1.read(); // Read the character from the WiFi board
      response += c;
      if (response == "valid")
      {
        validCar = true;
      }
      if (response == "invalid")
      {
        validCar = false;
      }
    }
  }

  Serial.print(response);

  return response;
}

/**
 * Displays the text in the center of the OLED display
 * @param text The text to be displayed
 */
void displayDisplayCenter(String text)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.clearDisplay();
  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  display.println(text);
  display.display();
}

void setup()
{
  pinMode(ENTRY_RED, OUTPUT);
  pinMode(ENTRY_GREEN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTION_SENSOR, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  servo.attach(SERVO_PIN);
  servo.write(0);

  Serial.begin(115200);
  SPI.begin();
  delay(2500);

  RC522_EntryGate.PCD_Init();
  delay(1500);
  RC522_Bay.PCD_Init();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
      ;
  }

  delay(10000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.display();
  Serial1.begin(115200);
}
void loop()
{
  servo.write(0);

  digitalWrite(ENTRY_RED, HIGH);
  digitalWrite(ENTRY_GREEN, LOW);

  display.fillRect(0, 16, SCREEN_WIDTH, 48, SSD1306_BLACK);
  displayDisplayCenter("Welcome");

  if (Serial1.available())
  {
    Serial.print("buffer: ");
    String espBuf;
    long int time = millis();

    while ((time + 1000) > millis())
    {
      while (Serial1.available())
      {
        char c = Serial1.read();
        espBuf += c;
      }
    }
    Serial.print(espBuf);
    Serial.println(" endbuffer");
  }

  double distance_cm = calcDist();
  // Checks if the RFID tag is present in the Entry Gate
  if (RC522_EntryGate.PICC_IsNewCardPresent() && RC522_EntryGate.PICC_ReadCardSerial())
  {
    Serial.println("Card detected:");

    String rfidTag = "";
    for (uint8_t i = 0; i < RC522_EntryGate.uid.size; i++)
    {
      rfidTag += String(RC522_EntryGate.uid.uidByte[i], HEX);
    }
    Serial.println(rfidTag);
    // Distance less than 50 indicates car is present in front of the gate
    if (distance_cm <= 50)
    {
      String preparedData = prepareDataForEntryGate(rfidTag, "entrygate/verify");
      Serial.println(preparedData);
      sendDataToWiFiBoard(preparedData, 10000);
      if (validCar)
      {
        displayDisplayCenter("GO");
        servo.write(90);
        digitalWrite(ENTRY_RED, LOW);
        digitalWrite(ENTRY_GREEN, HIGH);
        delay(2000);
      }
      else
      {
        displayDisplayCenter("Invalid Car");
      }
    }
    else
    {
      displayDisplayCenter("Come Closer");
    }
  }

  RC522_Bay.PCD_Init();
  // Check if a car is parked in the bay
  if (RC522_Bay.PICC_IsNewCardPresent() && RC522_Bay.PICC_ReadCardSerial())
  {
    Serial.println("Card from bay detected:");

    String rfidTag = "";
    for (uint8_t i = 0; i < RC522_Bay.uid.size; i++)
    {
      rfidTag += String(RC522_Bay.uid.uidByte[i], HEX);
    }
    Serial.println(rfidTag);

    String preparedData = prepareDataForBay(rfidTag, "bay/verify", "2", "true");
    Serial.println(preparedData);
    sendDataToWiFiBoard(preparedData, 1000);
  }
  else
  {
    String preparedData = prepareDataForBay("", "bay/verify", "2", "false");
    Serial.println(preparedData);
    sendDataToWiFiBoard(preparedData, 1000);
  }

  // Check if low light and motion detected
  if (analogRead(LIGHT_SENSOR) < 200 && digitalRead(MOTION_SENSOR) == 1)
  {
    Serial.println("Light Low and motion Detected");
    String preparedData = prepareDataForAdmin("Low light and Motion detected", "admin/warn");
    Serial.println(preparedData);
    sendDataToWiFiBoard(preparedData, 2000);
    for (int i = 0; i < 5; i++)
    {
      tone(BUZZER_PIN, 1000);
      delay(500);
      tone(BUZZER_PIN, 500);
      delay(500);
    }
    noTone(BUZZER_PIN);
  }
  delay(2000);
}

// ***********************
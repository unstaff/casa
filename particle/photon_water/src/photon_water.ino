// This #include statement was automatically added by the Particle IDE.
// this file is planterlevel4.ino in the web IDE

#include <Adafruit_SSD1306.h>

#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/*********** PIN OUTS BEGINS */
#define SOIL_PIN A0
#define POWER_PIN D6
/* PIN OUTS ENDS */

/*********** kConstants BEGINS */
#define kMINUTE 60 * 1000.0
// Min * Second * Millisec -> 120min
#define kINTERVAL 60 * kMINUTE
#define kMAXHUMIDITY 4095.0
// Below the device will start droping water
#define kMOISTURE_THRESHOLD 28
#define kNB_RUN_AVG 5
/* kConstants ENDS */

/*********** kGlobals BEGINS */
// sets time since last publish.
unsigned int last_publish = 0;
// last moisture read..
float last_moisture;
/* kGlobals ENDS */

void setup() {
  // Serial.begin(9600);
  Particle.function("Publish", PublishData);
  Particle.function("Moisture", GetMoisture);
  // Subscribe to the integration response event
  Particle.subscribe("hook-response/moisture", PubSubResponseHandler,
                     MY_DEVICES);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(SOIL_PIN, INPUT);
  digitalWrite(POWER_PIN, LOW);
  last_moisture = RecomputeAverageMoisture();

  // by default, we'll generate the high voltage from the 3.3v line internally!
  // (neat!)
  display.begin(SSD1306_SWITCHCAPVCC,
                0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)

  Serial.println(F("Start"));
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text
  display.setCursor(0, 20);     // Start at top-left corner
  display.println(F("Begin Moisture:"));
  display.setCursor(30, 30);
  display.println(last_moisture);

  display.display();
  delay(2000);

  PublishData(String(last_moisture, 2));
}

void loop() {
  unsigned long now = millis();
  if ((now - last_publish) < kINTERVAL) {
    // sleep 1 sec
    delay(1000);
    return;
  }

  // make sure the moisture webhook is setup properly.
  GetMoisture("");
  // resets last_publish variable to now (resets timer)
  last_publish = now;
}

int PublishData(String moistValue) {
  Particle.publish("DEBUG", "Published moisture:" + moistValue);
  Particle.publish("moisture", moistValue, PRIVATE);
  return 0;
}

void PubSubResponseHandler(const char *event, const char *data) {
  // Handle the integration response
  Particle.publish(
      "DEBUG",
      String::format("PubSubResponseHandler got back: event:%s  data:%s",
                     *event, *data));
}

int GetMoisture(String dummy_var) {
  last_moisture = RecomputeAverageMoisture();
  PublishData(String(last_moisture, 2));

  display.clearDisplay();
  display.setCursor(0, 0);
  // if (!Time.isValid()) {
  //     Particle.syncTime();
  //     waitUntil(Particle.syncTimeDone);
  // }
  display.println(Time.timeStr());
  display.setCursor(0, 20);
  display.println(F("Moisture:"));
  display.setCursor(0, 40);
  display.println(last_moisture);
  display.display();
  return 0;
}

float RecomputeAverageMoisture() {
  float avg_moisture = 0;
  int readings = 0;
  digitalWrite(POWER_PIN, HIGH);
  delay(250);
  for (int i = 0; i < kNB_RUN_AVG; ++i) {
    // forward
    delay(50);
    float moistValue = ((float)analogRead(SOIL_PIN) * (100.0 / kMAXHUMIDITY));
    avg_moisture += moistValue;
    readings++;
  }
  digitalWrite(POWER_PIN, LOW);
  avg_moisture /= readings;
  Particle.publish("DEBUG", String::format("Moisture:%f2.2", avg_moisture));
  return avg_moisture;
}
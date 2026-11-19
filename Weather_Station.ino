#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
byte bmpAddr = 0;

unsigned long cycles = 0;
unsigned long dhtOk = 0, dhtFail = 0;
unsigned long bmpOk = 0, bmpFail = 0;

byte ping(byte addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission();
}

int readChipID(byte addr) {
  Wire.beginTransmission(addr);
  Wire.write(0xD0);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom((int)addr, 1) != 1) return -1;
  return Wire.read();
}

void setup() {
  Serial.begin(9600);
  Serial.println("DHT22 + BMP280 combined test");

  // ---- DHT22 ----
  pinMode(DHTPIN, INPUT_PULLUP);
  delay(100);
  Serial.print("DHT22 DATA idle level: ");
  Serial.println(digitalRead(DHTPIN));  // should be 1
  dht.begin();

  // ---- BMP280 ----
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000, true);
#endif
  if (ping(0x76) == 0) bmpAddr = 0x76;
  else if (ping(0x77) == 0) bmpAddr = 0x77;

  if (bmpAddr == 0) {
    Serial.println("BMP280: not found at 0x76/0x77. Check 3.3V, GND, SDA (A4), SCL (A5).");
  } else {
    int id = readChipID(bmpAddr);
    Serial.print("BMP280 at 0x"); Serial.print(bmpAddr, HEX);
    Serial.print(", chip ID 0x"); Serial.println(id, HEX);  // 0x58 = BMP280
    if (!bmp.begin(bmpAddr, id)) {
      Serial.println("BMP280: library init failed.");
      bmpAddr = 0;
    }
  }

  Serial.println("Starting readings...\n");
  delay(2000);  // let the DHT22 settle
}

void loop() {
  cycles++;

  // ---- DHT22 ----
  float h = dht.readHumidity();
  float tD = dht.readTemperature();
  bool dhtGood = !(isnan(h) || isnan(tD)) && h >= 0 && h <= 100 && tD > -40 && tD < 80;
  if (dhtGood) dhtOk++; else dhtFail++;

  // ---- BMP280 ----
  float tB = NAN, p = NAN;
  bool bmpGood = false;
  if (bmpAddr != 0 && ping(bmpAddr) == 0) {
    tB = bmp.readTemperature();
    p = bmp.readPressure() / 100.0;
    bmpGood = !(isnan(tB) || isnan(p)) && tB > -40 && tB < 85 && p > 300 && p < 1100;
  }
  if (bmpGood) bmpOk++; else bmpFail++;

  // ---- Print ----
  Serial.print("DHT22: ");
  if (dhtGood) {
    Serial.print(tD, 1); Serial.print(" C, ");
    Serial.print(h, 1);  Serial.print(" %");
  } else {
    Serial.print("FAIL");
  }

  Serial.print("  |  BMP280: ");
  if (bmpGood) {
    Serial.print(tB, 2); Serial.print(" C, ");
    Serial.print(p, 2);  Serial.print(" hPa");
  } else {
    Serial.print("FAIL");
  }
  Serial.println();

  // Summary every 10 cycles
  if (cycles % 10 == 0) {
    Serial.print("[Summary] DHT ok/fail: ");
    Serial.print(dhtOk); Serial.print("/"); Serial.print(dhtFail);
    Serial.print("   BMP ok/fail: ");
    Serial.print(bmpOk); Serial.print("/"); Serial.println(bmpFail);
    if (dhtGood && bmpGood) {
      Serial.print("[Check] temp difference DHT vs BMP: ");
      Serial.print(tD - tB, 2);
      Serial.println(" C (should be within ~1-2 C)");
    }
  }

  delay(2000);
}

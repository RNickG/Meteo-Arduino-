// метеостанция на oled 64x128, dht22, bmp085

// bmp085
// SDA = ANALOG 4
// SCL = ANALOG 5

#include "SSD1306Ascii.h"
#include "SSD1306AsciiSoftSpi.h"
#include <dhtnew.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>

// pin definitions
#define OLED_CS_PIN    7
#define OLED_RST_PIN   8
#define OLED_DC_PIN    9
#define OLED_MOSI_PIN 11
#define OLED_CLK_PIN  10

#define DHT_PIN  4
#define DHT_TYPE 22

#define RTN_CHECK 1
#define TICKER_DELAY 25
#define GET_DHT_DELAY 2000
#define GET_BMP_DELAY 3600000
//#define GET_BMP_DELAY 1000

#define P_HISTORY_DEPTH 25
TickerState state;

SSD1306AsciiSoftSpi oled;
DHTNEW dht(DHT_PIN);
Adafruit_BMP085 bmp;
//------------------------------------------------------------------------------

int EEMEM pressure_addr[P_HISTORY_DEPTH+1];

//------------------------------------------------------------------------------
int pressure[P_HISTORY_DEPTH];

void printPressureHistory(void);

void setup() {
  EEPROM.get((int)&pressure_addr, pressure);
  
  oled.begin(&Adafruit128x64, OLED_CS_PIN, OLED_DC_PIN, OLED_CLK_PIN, OLED_MOSI_PIN, OLED_RST_PIN);
  oled.setFont(lcd5x7);
  oled.clear();
   
  printPressureHistory();
  
  oled.tickerInit(&state, lcd5x7, 0, true);\
  
  dht.setType(DHT_TYPE);

  bmp.begin(BMP085_STANDARD);

}
//------------------------------------------------------------------------------
uint32_t tickTime = 0;
uint32_t getDHTTime = 0;
uint32_t getBMPTime = GET_BMP_DELAY;
uint32_t currMills;

String ticker;



const unsigned char hours[4] = {3, 6, 12, P_HISTORY_DEPTH - 1};
//------------------------------------------------------------------------------

void loop() {

  currMills = millis();

  // чтение температуры и влажности и запись в строку тикера
  if (getDHTTime <= currMills) {
    getDHTTime = currMills + GET_DHT_DELAY;

    int chk = dht.read();
    if (chk != DHTLIB_WAITING_FOR_READ) {
      ticker = "% t: " + String(dht.getTemperature(), 1) + "C, H: " + String(dht.getHumidity(), 1);
    }

    double p = 0;
    modf(bmp.readPressure() / 133.3, &p);
    pressure[0] = int(p);

    oled.set2X();

    char pDir = ' ';
    if (pressure[12] != 0) {
      if (pressure[0] < pressure[12]) {
        pDir = 'v';
      }
      if (pressure[0] > pressure[12]) {
        pDir = '^';
      }
      else {
        pDir = '-';
      }
    }

    String pString = "P: " + String(pressure[0]) + "mm " + pDir;
    oled.setCursor(0, 2);
    oled.print(pString);

  }

  // чтение давления
  if (getBMPTime <= currMills) {
    getBMPTime = currMills + GET_BMP_DELAY;

    for (int i = P_HISTORY_DEPTH - 1; i > 0 ; i--) {
      pressure[i] = pressure[i - 1];
    }

    EEPROM.put((int)&pressure_addr, pressure);

    printPressureHistory();

  }

  //  обновление тикера
  if (tickTime <= currMills) {
    tickTime = currMills + TICKER_DELAY;

    // Should check for error. rtn < 0 indicates error.
    int8_t rtn = oled.tickerTick(&state);

    // See above for definition of RTN_CHECK.
    if (rtn <= RTN_CHECK) {
      // Should check for error. Return of false indicates error.
      oled.tickerText(&state, ticker);
    }
  }
}

void printPressureHistory() {
  oled.set1X();

  int pDelta;

  for (int i = 0; i < 4; i++) {
    if (pressure[hours[i]] != 0) {
      pDelta = pressure[0] - pressure[hours[i]];
      String pString = (hours[i] < 10 ? " " : "") +
                       String(hours[i]) + ": " + String(pressure[hours[i]]) + "mm "
                       +  (pDelta != 0 ? "| " + String(pDelta) + "    " : "     ");
      oled.setCursor(0, 4 + i);
      oled.print(pString);
    }
  }
}

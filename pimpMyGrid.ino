/**
 * PimpMyPowerMeter (2020)
 * Gorille70, tusti70@gmail.com
 * 
 * Drive your grid !
 * 
 */

#include "MemoryWebResources.h"
#include "EEPROMAnything.h"
#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];}))
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <ESPmDNS.h>
#include <pgmspace.h>
#include <math.h>
#include "EmonLib.h"
#include <driver/adc.h>
#include <Chrono.h>

#define ADC_INPUT_1 34
#define ADC_INPUT_2 35
#define ADC_INPUT_3 32
#define ADC_INPUT_4 33
#define ADC_INPUT_5 25
#define ADC_INPUT_6 26
#define ADC_INPUT_7 27
#define ADC_INPUT_8 14
#define ADC_INPUT_9 12
#define ADC_INPUT_10 13
#define LED_BUILTIN 2
#define BOOT_BUTTON 0


// Data structure
typedef struct {
  char myName[20];
  char emon1Title[20]; char emon1Coeff[10];
  char emon2Title[20]; char emon2Coeff[10];
  char emon3Title[20]; char emon3Coeff[10];
  char emon4Title[20]; char emon4Coeff[10];
  char emon5Title[20]; char emon5Coeff[10];
  char emon6Title[20]; char emon6Coeff[10];
  char emon7Title[20]; char emon7Coeff[10];
  char emon8Title[20]; char emon8Coeff[10];
  char emon9Title[20]; char emon9Coeff[10];
  char emon10Title[20]; char emon10Coeff[10];
  char urlInfluxDb[50];
  char portInfluxDb[5];
  
  char voltCalibrationValue[10]; char phaseShift[4];
  char currentCalibrationValue[10];
  
  char thresold[5];
  char anIP[20];
  char aNetmask[20];
  char aGateway[20];
  char aWifiMode[20];
  char aIpConfig[20];
  char ssid[20];
  char password[20];
  char extension[1];
  uint16_t crc;
} Configuration __attribute__ ((packed));


// The configuration object
Configuration pimpGrid;

// Ip Adress Object
IPAddress ip;
// The Web server on 80 port
WebServer server(80);
// Udp connection to influxDb
WiFiUDP udp;


// Some values
String ledValue="off";
String version="1.0";
String host="PimpMyGrid";
String jsonScanWifi="";
int ledState = LOW;
int displayMode,loopMode0,extension,loopMode,sendUdp=0;

// A Chronograph 
Chrono myChrono;
unsigned long previousMillis = 0;
unsigned long previousMillisWifi = 0;
const long interval = 1000;
const long intervalWifi = 60000;

// The ten monitors
EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;
EnergyMonitor emon4;
EnergyMonitor emon5;
EnergyMonitor emon6;
EnergyMonitor emon7;
EnergyMonitor emon8;
EnergyMonitor emon9;
EnergyMonitor emon10;

double treshold=1;
double watt1,watt2,watt3,watt4,watt5,watt6,watt7,watt8,watt9,watt10,total,sum,avg,last;
unsigned long lastMeasurement = 0;

void setup()
{
 Serial.begin(9600);
 analogReadResolution(ADC_BITS);
 // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
  Serial.println(F("SSD1306 allocation failed"));
  for(;;); // Don't proceed, loop forever
 }
 Serial.println("Booting PimpMyGrid...");displayTitle("Booting PimpMyGrid...");delay(250);
 
 Serial.println("Reading Parameters...");displayTitle("Reading Parameters...");delay(250);
 readParameters();
 
 
 Serial.println("Setup Wifi...");displayTitle("Setup Wifi...");delay(250);
 setupWifi();
 
 pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

 Serial.println("Calibrating monitor entries...");displayTitle("Calibrating monitor entries...");delay(250);
 emon1.voltage(ADC_INPUT_1, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon1.current(ADC_INPUT_1, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon2.voltage(ADC_INPUT_2, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon2.current(ADC_INPUT_2, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon3.voltage(ADC_INPUT_3, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon3.current(ADC_INPUT_3, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon4.voltage(ADC_INPUT_4, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon4.current(ADC_INPUT_4, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon5.voltage(ADC_INPUT_5, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon5.current(ADC_INPUT_5, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon6.voltage(ADC_INPUT_6, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon6.current(ADC_INPUT_6, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon7.voltage(ADC_INPUT_7, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon7.current(ADC_INPUT_7, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon8.voltage(ADC_INPUT_8, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon8.current(ADC_INPUT_8, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon9.voltage(ADC_INPUT_9, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift)); 
 emon9.current(ADC_INPUT_9, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value
 emon10.voltage(ADC_INPUT_10, atof(pimpGrid.voltCalibrationValue), atof(pimpGrid.phaseShift));  
 emon10.current(ADC_INPUT_10, atof(pimpGrid.currentCalibrationValue));  //pin for current measurement, calibration value

 Serial.println("System ready, wait");displayTitle("System ready, wait");delay(500);

 displayLogo();delay(500);
}


// Method for setting WIFI (Access Point or Station)
void setupWifi()
{
  uint32_t brown_reg_temp = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG); //save WatchDog register
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
   
  scanWifi();
  Serial.println(" - Setting up wifi...");
  if ( strcmp(pimpGrid.aWifiMode, "AP")  == 0) 
  {
    Serial.println("    * Set access point mode");
    setAPWifi();
  }else
  {
    Serial.println("    * Set station mode");
    setStationWifi();
  }
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, brown_reg_temp); //enable brownout detector
}

// Reset config, to factory settings
void resetConfig()
{
  Serial.println("Reset SSID");
  sprintf_P(pimpGrid.ssid, "PimpMyGrid-%06X", ESP.getEfuseMac());
  sprintf_P(pimpGrid.myName, "PimpMyGrid-%06X", ESP.getEfuseMac());
  Serial.println("Reset anIP");
  strcpy_P(pimpGrid.anIP, "192.168.4.1");
  Serial.println("Reset aNetmask");
  strcpy_P(pimpGrid.aNetmask, "255.255.255.0");
  Serial.println("Reset aGateway");
  strcpy_P(pimpGrid.aGateway, "192.168.4.254");
  Serial.println("Reset aWifiMode");
  strcpy_P(pimpGrid.aWifiMode, "AP");
  Serial.println("Reset aIpConfig");
  strcpy_P(pimpGrid.aIpConfig,"STATIC");
  Serial.println("Reset password");
  strcpy_P(pimpGrid.password, "");
  Serial.println("Reset emon titles");
  strcpy_P(pimpGrid.emon1Title,"Emon1 Title");
  strcpy_P(pimpGrid.emon2Title,"Emon2 Title");
  strcpy_P(pimpGrid.emon3Title,"Emon3 Title");
  strcpy_P(pimpGrid.emon4Title,"Emon5 Title");
  strcpy_P(pimpGrid.emon5Title,"Emon5 Title");
  strcpy_P(pimpGrid.emon6Title,"Emon6 Title");
  strcpy_P(pimpGrid.emon7Title,"Emon7 Title");
  strcpy_P(pimpGrid.emon8Title,"Emon8 Title");
  strcpy_P(pimpGrid.emon9Title,"Emon9 Title");
  strcpy_P(pimpGrid.emon10Title,"Emon10 Title");
  Serial.println("Reset emon coeff");
  strcpy_P(pimpGrid.emon1Coeff,"1.0");
  strcpy_P(pimpGrid.emon2Coeff,"1.0");
  strcpy_P(pimpGrid.emon3Coeff,"1.0");
  strcpy_P(pimpGrid.emon4Coeff,"1.0");
  strcpy_P(pimpGrid.emon5Coeff,"1.0");
  strcpy_P(pimpGrid.emon6Coeff,"1.0");
  strcpy_P(pimpGrid.emon7Coeff,"1.0");
  strcpy_P(pimpGrid.emon8Coeff,"1.0");
  strcpy_P(pimpGrid.emon9Coeff,"1.0");
  strcpy_P(pimpGrid.emon10Coeff,"1.0");
  Serial.println("Reset voltCalibrationValue");
  strcpy_P(pimpGrid.voltCalibrationValue,"850");
  strcpy_P(pimpGrid.phaseShift,"1.7");
  strcpy_P(pimpGrid.currentCalibrationValue,"34");
  Serial.println("Reset influxDb Parameters");
  strcpy_P(pimpGrid.urlInfluxDb,"");
  strcpy_P(pimpGrid.portInfluxDb,"");
  Serial.println("Reset extension bridge");
  strcpy_P(pimpGrid.extension,"0");
}

// Read Parameters
void readParameters()
{
   EEPROM.begin(1024);
   if (readConfig(false)) {
    Serial.println("Good configuration");
   }else
   {
    Serial.println("Bad configuration, restore factory settings");
    memset(&pimpGrid, 0, sizeof(Configuration));
    resetConfig();
    saveConfig();
   }
   showConfig();
}

// Display configuration to COM PORT
void showConfig() 
{
  Serial.println("***** Name"); 
  Serial.print("ssid     : "); Serial.println(pimpGrid.myName); 
  Serial.println("***** Wifi"); 
  Serial.print("ssid     : "); Serial.println(pimpGrid.ssid); 
  Serial.print("passwd   : "); Serial.println(pimpGrid.password); 
  Serial.print("wifiMode : "); Serial.println(pimpGrid.aWifiMode); 
  Serial.println("\r\n****** Network"); 
  Serial.print("ip       : "); Serial.println(pimpGrid.anIP); 
  Serial.print("netmask  : "); Serial.println(pimpGrid.aNetmask); 
  Serial.print("gateway  : "); Serial.println(pimpGrid.aGateway); 
  Serial.print("ipconfig : "); Serial.println(pimpGrid.aIpConfig); 
  Serial.println("\r\n****** Emon data"); 
  Serial.print("Emon1  Title   :") ; Serial.println(pimpGrid.emon1Title); 
  Serial.print("Emon2  Title   :") ; Serial.println(pimpGrid.emon2Title); 
  Serial.print("Emon3  Title   :") ; Serial.println(pimpGrid.emon3Title); 
  Serial.print("Emon4  Title   :") ; Serial.println(pimpGrid.emon4Title); 
  Serial.print("Emon5  Title   :") ; Serial.println(pimpGrid.emon5Title); 
  Serial.print("Emon6  Title   :") ; Serial.println(pimpGrid.emon6Title); 
  Serial.print("Emon7  Title   :") ; Serial.println(pimpGrid.emon7Title); 
  Serial.print("Emon8  Title   :") ; Serial.println(pimpGrid.emon8Title); 
  Serial.print("Emon9  Title   :") ; Serial.println(pimpGrid.emon9Title); 
  Serial.print("Emon10 Title   :") ; Serial.println(pimpGrid.emon10Title); 
  Serial.println("\r\n****** Emon coeff"); 
  Serial.print("Emon1  Coeff   :") ; Serial.println(pimpGrid.emon1Coeff); 
  Serial.print("Emon2  Coeff   :") ; Serial.println(pimpGrid.emon2Coeff); 
  Serial.print("Emon3  Coeff   :") ; Serial.println(pimpGrid.emon3Coeff); 
  Serial.print("Emon4  Coeff   :") ; Serial.println(pimpGrid.emon4Coeff); 
  Serial.print("Emon5  Coeff   :") ; Serial.println(pimpGrid.emon5Coeff); 
  Serial.print("Emon6  Coeff   :") ; Serial.println(pimpGrid.emon6Coeff); 
  Serial.print("Emon7  Coeff   :") ; Serial.println(pimpGrid.emon7Coeff); 
  Serial.print("Emon8  Coeff   :") ; Serial.println(pimpGrid.emon8Coeff); 
  Serial.print("Emon9  Coeff   :") ; Serial.println(pimpGrid.emon9Coeff); 
  Serial.print("Emon10 Coeff   :") ; Serial.println(pimpGrid.emon10Coeff); 
  Serial.println("\r\n****** Calibrations"); 
  Serial.print("voltCalibrationValue    :") ; Serial.println(pimpGrid.voltCalibrationValue); 
  Serial.print("phaseShift              :") ; Serial.println(pimpGrid.phaseShift); 
  Serial.print("currentCalibrationValue :") ; Serial.println(pimpGrid.currentCalibrationValue); 
  Serial.println("\r\n****** InfluxDb"); 
  Serial.print("urlInfluxDb       :") ; Serial.println(pimpGrid.urlInfluxDb); 
  Serial.print("portInfluxDb  :") ; Serial.println(pimpGrid.portInfluxDb); 
  Serial.println("\r\n****** Extension module"); 
  Serial.print("extensionModule   :") ; Serial.println(pimpGrid.extension); 
  Serial.println("");
}

// Save configuration
bool saveConfig (void) 
{
  Serial.println("Saving Configuration");
  uint8_t * pconfig ;
  bool ret_code;
  pconfig = (uint8_t *) &pimpGrid ;
  pimpGrid.crc = ~0;
  for (uint16_t i = 0; i < sizeof (Configuration) - 2; ++i)
    pimpGrid.crc = crc16Update(pimpGrid.crc, *pconfig++);
  pconfig = (uint8_t *) &pimpGrid ;
  for (uint16_t i = 0; i < sizeof(Configuration); ++i) 
  EEPROM.write(i, *pconfig++);
  EEPROM.commit();
  ret_code = readConfig(false);
  Serial.print(F("Write the configuration "));
  if (ret_code)
    Serial.println(F("OK!"));
  else
    Serial.println(F("Error!"));
  return (ret_code);
}

// Read Configuration 
bool readConfig (bool clear_on_error) 
{
  uint16_t crc = ~0;
  uint8_t * pconfig = (uint8_t *) &pimpGrid ;
  uint8_t data ;
  for (uint16_t i = 0; i < sizeof(Configuration); ++i) {
    data = EEPROM.read(i);
    *pconfig++ = data ;
    crc = crc16Update(crc, data);
  }
  if (crc != 0) {
    if (clear_on_error)
      memset(&pimpGrid, 0, sizeof( Configuration ));
    return false;
  }
  return true ;
}

// Update CRC
uint16_t crc16Update(uint16_t crc, uint8_t a)
{
  int i;
  crc ^= a;
  for (i = 0; i < 8; ++i)  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }
  return crc;
}

// Led Request processing, allow on, off and blinking
void processLed()
  {
        ledValue=server.arg("state");
        if (ledValue == "on") digitalWrite(LED_BUILTIN, LOW);
        else if (ledValue == "off") digitalWrite(LED_BUILTIN, HIGH);
        server.send(200, "text/plain", "Led is now " + ledValue);
        Serial.print("Led is now ");
        Serial.println(ledValue);
  }

void displayLogo()
{
  display.clearDisplay();
  display.display();
  display.setCursor(0,10);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  display.println("PimpMy Grid");
  display.display();
  display.setCursor(55,80);
  display.setTextSize(1);
  display.println(version);
  display.display();
}

void displayTitle(String text)
{
  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.println(text);
  display.display();
  //display.startscrollright(0x00, 0x07);
}

void screenOff()
{
  display.clearDisplay();
  displayTitle("Off in 1 second");
  delay(1000);
  display.display();
  display.clearDisplay();
  display.display();
}

void displayConfiguration()
{
  display.clearDisplay();
  display.display();
  display.stopscroll();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Volt/Current conf.");
  display.display();
  display.setCursor(0,16);
  display.println("V.cal  :" + String(pimpGrid.voltCalibrationValue));
  display.setCursor(0,24);
  display.println("P.shift:" + String(pimpGrid.phaseShift));
  display.setCursor(0,32);
  display.println("C.cal  :" + String(pimpGrid.currentCalibrationValue));
  display.setCursor(0,40);
  display.println("I.Url  :" +  String(pimpGrid.urlInfluxDb));
  display.setCursor(0,48);
  display.println("I.Port :" +  String(pimpGrid.portInfluxDb) );
  display.setCursor(0,56);
  display.println("Device :" +  String(pimpGrid.myName));

  display.startscrollright(0x07, 0x07);
  display.display();
}

void displayWifiConfiguration()
{
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("System/Wifi conf.");
  display.display();
  display.setCursor(0,16);
  display.println("ip   :" + String(pimpGrid.anIP));
  display.setCursor(0,24);
  display.println("netm :" + String(pimpGrid.aNetmask));
  display.setCursor(0,32);
  display.println("gate :" + String(pimpGrid.aGateway));
  display.setCursor(0,40);
  display.println("wifiM:" + String(pimpGrid.aWifiMode));
  display.setCursor(0,48);
  display.println(String(pimpGrid.ssid));
  display.startscrollright(0x06, 0x07);
  display.display();
}

void displayWatt(String watt1,String watt2,String watt3,String watt4,String watt5,String watt6,String watt7,String watt8,String watt9,String watt10,String total)
{
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Total : " + total +"w");
  display.setCursor(0,16);
  display.println("1:" +watt1 +"w");
  display.setCursor(0,24);
  display.println("2:" +watt2 +"w");
  display.setCursor(0,32);
  display.println("3:" +watt3 +"w");
  display.setCursor(0,40);
  display.println("4:" +watt4 +"w");
  display.setCursor(0,48);
  display.println("5:" +watt5 +"w");
  display.setCursor(60,16);
  display.println("6:" +watt6 +"w");
  display.setCursor(60,24);
  display.println("7:" +watt7 +"w");
  display.setCursor(60,32);
  display.println("8:" +watt8 +"w");
  display.setCursor(60,40);
  display.println("9:" +watt9 +"w");
  display.setCursor(60,48);
  display.println("10:" +watt10 +"w");
  display.display();
  //display.startscrollright(0x00, 0x07);
}

// Kowned informations process, returns as JSON format
void processInformations()
  {
        long elapsed = myChrono.elapsed();
        String json="{";
        json=json +"\"version\":\"" + version +"\",";
        json=json +"\"starttime\":" + elapsed + ",";
        json=json +"\"deviceName\":\"" + pimpGrid.myName + "\",";
        json=json +"\"ip\":\"" + ipToString(WiFi.localIP()) + "\",";
        json=json +"\"netmask\":\"" + pimpGrid.aNetmask + "\",";
        json=json +"\"gateway\":\"" + pimpGrid.aGateway + "\",";
        json=json +"\"wifimode\":\"" + pimpGrid.aWifiMode + "\",";
        json=json +"\"ipconfig\":\"" + pimpGrid.aIpConfig + "\",";
        json=json +"\"ssid\":\"" + pimpGrid.ssid + "\",";
        json=json +"\"password\":\"" + pimpGrid.password + "\",";

        json=json +"\"emon1Title\":\"" + pimpGrid.emon1Title + "\",";
        json=json +"\"emon2Title\":\"" + pimpGrid.emon2Title + "\",";
        json=json +"\"emon3Title\":\"" + pimpGrid.emon3Title + "\",";
        json=json +"\"emon4Title\":\"" + pimpGrid.emon4Title + "\",";
        json=json +"\"emon5Title\":\"" + pimpGrid.emon5Title + "\",";
        json=json +"\"emon6Title\":\"" + pimpGrid.emon6Title + "\",";
        json=json +"\"emon7Title\":\"" + pimpGrid.emon7Title + "\",";
        json=json +"\"emon8Title\":\"" + pimpGrid.emon8Title + "\",";
        json=json +"\"emon9Title\":\"" + pimpGrid.emon9Title + "\",";
        json=json +"\"emon10Title\":\"" + pimpGrid.emon10Title + "\",";

        json=json +"\"emon1Coeff\":\"" + pimpGrid.emon1Coeff + "\",";
        json=json +"\"emon2Coeff\":\"" + pimpGrid.emon2Coeff + "\",";
        json=json +"\"emon3Coeff\":\"" + pimpGrid.emon3Coeff + "\",";
        json=json +"\"emon4Coeff\":\"" + pimpGrid.emon4Coeff + "\",";
        json=json +"\"emon5Coeff\":\"" + pimpGrid.emon5Coeff + "\",";
        json=json +"\"emon6Coeff\":\"" + pimpGrid.emon6Coeff + "\",";
        json=json +"\"emon7Coeff\":\"" + pimpGrid.emon7Coeff + "\",";
        json=json +"\"emon8Coeff\":\"" + pimpGrid.emon8Coeff + "\",";
        json=json +"\"emon9Coeff\":\"" + pimpGrid.emon9Coeff + "\",";
        json=json +"\"emon10Coeff\":\"" + pimpGrid.emon10Coeff + "\",";

        json=json +"\"emon1Watts\":\"" + watt1 + "\",";
        json=json +"\"emon2Watts\":\"" + watt2 + "\",";
        json=json +"\"emon3Watts\":\"" + watt3 + "\",";
        json=json +"\"emon4Watts\":\"" + watt4 + "\",";
        json=json +"\"emon5Watts\":\"" + watt5 + "\",";
        json=json +"\"emon6Watts\":\"" + watt6 + "\",";
        json=json +"\"emon7Watts\":\"" + watt7 + "\",";
        json=json +"\"emon8Watts\":\"" + watt8 + "\",";
        json=json +"\"emon9Watts\":\"" + watt9 + "\",";
        json=json +"\"emon10Watts\":\"" + watt10 + "\",";
        json=json +"\"total\":\"" + total + "\",";

        json=json +"\"voltCalibrationValue\":\"" + pimpGrid.voltCalibrationValue + "\",";
        json=json +"\"phaseShift\":\"" + pimpGrid.phaseShift + "\",";
        json=json +"\"currentCalibrationValue\":\"" + pimpGrid.currentCalibrationValue + "\",";

        
        json=json +"\"urlInfluxDb\":\"" + pimpGrid.urlInfluxDb + "\",";
        json=json +"\"portInfluxDb\":\"" + pimpGrid.portInfluxDb + "\",";
        json=json +"}";
        server.send(200, "text/plain", json);
}

// Set mode to access point wifi
void setAPWifi()
{
  
  Serial.println("    * Setting wifi access point starting by reseting factory settings");
  resetConfig();
  strcpy_P(pimpGrid.aWifiMode, "AP");
  if ( strcmp(pimpGrid.password, "") != 0 )
  {
    WiFi.softAP((const char*)pimpGrid.ssid,(const char*)pimpGrid.password);  
  }else
  {
    WiFi.softAP((const char*)pimpGrid.ssid);
  }
  
  int A_IP=atoi(subStr(pimpGrid.anIP, ".", 1));int B_IP=atoi(subStr(pimpGrid.anIP, ".", 2));int C_IP=atoi(subStr(pimpGrid.anIP, ".", 3));int D_IP=atoi(subStr(pimpGrid.anIP, ".", 4));
  int A_GW=atoi(subStr(pimpGrid.aGateway, ".", 1));int B_GW=atoi(subStr(pimpGrid.aGateway, ".", 2));int C_GW=atoi(subStr(pimpGrid.aGateway, ".", 3));int D_GW=atoi(subStr(pimpGrid.aGateway, ".", 4));
  int A_SN=atoi(subStr(pimpGrid.aNetmask, ".", 1));int B_SN=atoi(subStr(pimpGrid.aNetmask, ".", 2));int C_SN=atoi(subStr(pimpGrid.aNetmask, ".", 3));int D_SN=atoi(subStr(pimpGrid.aNetmask, ".", 4));

  IPAddress ip(A_IP, B_IP, C_IP, D_IP);
  IPAddress gateway(A_GW, B_GW, C_GW, D_GW);
  IPAddress subnet(A_SN, B_SN, C_SN, D_SN);
  WiFi.softAPConfig(ip, gateway, subnet);
  ip=WiFi.softAPIP();
  manageServerHttp();
  Serial.print("    * Ready in AP Wifi mode with SSID: [");
  Serial.print(pimpGrid.ssid);
  Serial.print("] password: [");
  Serial.print(pimpGrid.password);Serial.println("]");
  Serial.print(" - Open http://");
  Serial.print(ip);
  Serial.println(" in your favorite browser");
  server.send(200, "text/plain", ipToString(ip));
}

// Set mode to Station wifi
void setStationWifi()
{
 Serial.println("    * Setting wifi station");
 strcpy_P(pimpGrid.aWifiMode, "ST");
 showConfig();
 WiFi.mode(WIFI_STA);
 if ( strcmp(pimpGrid.password, "") != 0 )
 WiFi.begin((const char*)pimpGrid.ssid, (const char*)pimpGrid.password);

 if ( strcmp(pimpGrid.aIpConfig, "STATIC") == 0 )
 {
  int A_IP=atoi(subStr(pimpGrid.anIP, ".", 1));int B_IP=atoi(subStr(pimpGrid.anIP, ".", 2));int C_IP=atoi(subStr(pimpGrid.anIP, ".", 3));int D_IP=atoi(subStr(pimpGrid.anIP, ".", 4));
  int A_GW=atoi(subStr(pimpGrid.aGateway, ".", 1));int B_GW=atoi(subStr(pimpGrid.aGateway, ".", 2));int C_GW=atoi(subStr(pimpGrid.aGateway, ".", 3));int D_GW=atoi(subStr(pimpGrid.aGateway, ".", 4));
  int A_SN=atoi(subStr(pimpGrid.aNetmask, ".", 1));int B_SN=atoi(subStr(pimpGrid.aNetmask, ".", 2));int C_SN=atoi(subStr(pimpGrid.aNetmask, ".", 3));int D_SN=atoi(subStr(pimpGrid.aNetmask, ".", 4));
 
  IPAddress ip(A_IP, B_IP, C_IP, D_IP);
  IPAddress gateway(A_GW, B_GW, C_GW, D_GW);
  IPAddress subnet(A_SN, B_SN, C_SN, D_SN);
 
  WiFi.config(ip, gateway, subnet);
 }
 
 uint8_t timeout = 600; 
 Serial.print("    * Waiting connection aknowledge .");    
 while ( WiFi.waitForConnectResult() != WL_CONNECTED && timeout )
   {
    Serial.print(".");
    delay(500);
    sendUdp=0;
    --timeout;
   }
 Serial.println(".");
 if ( timeout <= 0)
 {
  Serial.println("Wifi connection failed, check ssid and password, reseting to access point");
  resetConfig();
  saveConfig();
  server.send(200, "text/plain", "KO");
 }else
 {
  saveConfig();
  MDNS.begin((const char*)host.c_str());
  manageServerHttp();
  MDNS.addService("http", "tcp", 80);
  ip=WiFi.localIP();
  Serial.print("    * Ready in Station Wifi mode with SSID: [");
  strcpy_P(pimpGrid.aNetmask,ipToString(WiFi.subnetMask()).c_str());
  strcpy_P(pimpGrid.aGateway,ipToString(WiFi.gatewayIP()).c_str());
  
  Serial.print(pimpGrid.ssid);
  Serial.print("] password: [");
  Serial.print(pimpGrid.password);Serial.println("]");
  Serial.print(" - Open http://");
  Serial.print(ip);
  Serial.println(" in your favorite browser");
  server.send(200, "text/plain", ipToString(ip));
  sendUdp=1;
 }
}

// HTTP Server request Management
void manageServerHttp()
{
    // Head HTTP Management
    server.on("/", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.setContentLength(strlen_P(DATA_index));
      server.sendHeader("Cache-Control", "max-age=290304000, public");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendContent_P( DATA_index, sizeof(DATA_index) );
    });
    
    // When Epson Logo is requested
    server.on ("/pmg.jpg", HTTP_GET, [](){
      server.setContentLength(strlen_P(DATA_logo));
      server.sendHeader("Cache-Control", "max-age=290304000, public");
      server.sendContent_P( DATA_logo, sizeof(DATA_logo) );
    });

    // Clean reboot request
    server.on("/reboot", HTTP_GET, [](){
      Serial.println("Reboot asked..");
      ESP.restart();
    });

    // Led management request
    server.on("/led", [](){
      processLed();
    });

    // Display knowned informations
    server.on("/info", [](){
      processInformations();
    });
    
    // Display knowned informations JSON
    server.on("/json", [](){
      processInformations();
    });

    // Reset factory settings
    server.on("/factory", [](){
      resetConfig();
      saveConfig();
      showConfig();
    });

    // Save Settings
    server.on("/settings", [](){
      strcpy_P(pimpGrid.emon1Title, server.arg("emon1InputLabel").c_str());
      strcpy_P(pimpGrid.emon2Title, server.arg("emon2InputLabel").c_str());
      strcpy_P(pimpGrid.emon3Title, server.arg("emon3InputLabel").c_str());
      strcpy_P(pimpGrid.emon4Title, server.arg("emon4InputLabel").c_str());
      strcpy_P(pimpGrid.emon5Title, server.arg("emon5InputLabel").c_str());
      strcpy_P(pimpGrid.emon6Title, server.arg("emon6InputLabel").c_str());
      strcpy_P(pimpGrid.emon7Title, server.arg("emon7InputLabel").c_str());
      strcpy_P(pimpGrid.emon8Title, server.arg("emon8InputLabel").c_str());
      strcpy_P(pimpGrid.emon9Title, server.arg("emon9InputLabel").c_str());
      strcpy_P(pimpGrid.emon10Title, server.arg("emon10InputLabel").c_str());

      strcpy_P(pimpGrid.emon1Coeff, server.arg("emon1InputCoeff").c_str());
      strcpy_P(pimpGrid.emon2Coeff, server.arg("emon2InputCoeff").c_str());
      strcpy_P(pimpGrid.emon3Coeff, server.arg("emon3InputCoeff").c_str());
      strcpy_P(pimpGrid.emon4Coeff, server.arg("emon4InputCoeff").c_str());
      strcpy_P(pimpGrid.emon5Coeff, server.arg("emon5InputCoeff").c_str());
      strcpy_P(pimpGrid.emon6Coeff, server.arg("emon6InputCoeff").c_str());
      strcpy_P(pimpGrid.emon7Coeff, server.arg("emon7InputCoeff").c_str());
      strcpy_P(pimpGrid.emon8Coeff, server.arg("emon8InputCoeff").c_str());
      strcpy_P(pimpGrid.emon9Coeff, server.arg("emon9InputCoeff").c_str());
      strcpy_P(pimpGrid.emon10Coeff, server.arg("emon10InputCoeff").c_str());

      strcpy_P(pimpGrid.voltCalibrationValue, server.arg("voltCalibrationInput").c_str());
      strcpy_P(pimpGrid.phaseShift, server.arg("phaseShiftInput").c_str());
      strcpy_P(pimpGrid.currentCalibrationValue, server.arg("currentCalibrationInput").c_str());

      strcpy_P(pimpGrid.urlInfluxDb, server.arg("ipInfluxInput").c_str());
      strcpy_P(pimpGrid.portInfluxDb, server.arg("portInflux").c_str());
      
      strcpy_P(pimpGrid.extension, server.arg("extensionInput").c_str());

      strcpy_P(pimpGrid.myName, server.arg("myName").c_str());
      
      saveConfig();
      showConfig();
    });

    server.on("/scanWifi", [](){
      server.send(200, "text/plain", scanWifi());
    });
    
    // Wifi Access point management 
    server.on("/wifiAp", [](){
      strcpy_P(pimpGrid.ssid, server.arg("ssid").c_str());
      strcpy_P(pimpGrid.password, server.arg("passwd").c_str());
      strcpy_P(pimpGrid.aIpConfig, server.arg("ipconfig").c_str());
      setAPWifi();
    });

    // Wifi Station management
    server.on("/wifiStation",[](){
      strcpy_P(pimpGrid.ssid, server.arg("ssid").c_str());
      strcpy_P(pimpGrid.password, server.arg("passwd").c_str());
      strcpy_P(pimpGrid.aIpConfig, server.arg("ipconfig").c_str());
      setStationWifi();
    });
    
    // Network management (staticIP)
    server.on("/staticIp", [](){
      strcpy_P(pimpGrid.aGateway, server.arg("gateway").c_str());
      strcpy_P(pimpGrid.aNetmask, server.arg("netmask").c_str());
      strcpy_P(pimpGrid.anIP, server.arg("ip").c_str());
      strcpy_P(pimpGrid.aIpConfig, "STATIC");
      saveConfig();
      showConfig();
      Serial.println("Rebooting");
      ESP.restart();
    });

     // Network management (dhcpIP)
    server.on("/dhcpIp", [](){
      strcpy_P(pimpGrid.aGateway, "");
      strcpy_P(pimpGrid.aNetmask, "");
      strcpy_P(pimpGrid.anIP, "");
      strcpy_P(pimpGrid.aIpConfig, "DHCP");
      saveConfig();
      showConfig();
      Serial.println("Rebooting");
      ESP.restart();
    });


    // On update request, with upload file
    server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },[](){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        
        Serial.printf("Update: %s\n", upload.filename.c_str());
        // For esp32
        // add: #include <Update.h>
        // del: WiFiUDP::stopAll();
        // del: uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        // add: uint32_t maxSketchSpace = 0x140000;
        //uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        uint32_t maxSketchSpace = 0x140000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });

    // Starting webServer
    server.begin();
}


/**
 * Return a String from an IpObject
 */
String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

/**
 * Method for scanning wifi network SSID
 */
String scanWifi()
{
  Serial.println(" - Scanning wifi...");
  int Tnetwork = WiFi.scanNetworks();
  Serial.printf("    * %d network(s) found\n", Tnetwork);
  jsonScanWifi="{\"wifiNetworks\":[";
  for (int i = 0; i < Tnetwork; i++)
  {
    jsonScanWifi=jsonScanWifi + "{";
    jsonScanWifi=jsonScanWifi + "\"index\":" + (i+1) + ",";
    jsonScanWifi=jsonScanWifi + "\"name\":\"" + WiFi.SSID(i).c_str() + "\",";
    jsonScanWifi=jsonScanWifi + "\"channel\":" + WiFi.channel(i) + ",";
    jsonScanWifi=jsonScanWifi + "\"rssi\":" + WiFi.RSSI(i) + ",";
    jsonScanWifi=jsonScanWifi + "\"encrypt\":" + WiFi.encryptionType(i) + ",";
    jsonScanWifi=jsonScanWifi + "},";
  }
  jsonScanWifi=jsonScanWifi +"]}";
  jsonScanWifi.replace(",}","}");
  jsonScanWifi.replace(",]","]");
  Serial.println("    * " + jsonScanWifi);
  return jsonScanWifi;
}

double compute(EnergyMonitor em)
{
   sum=0;
   for ( int i=0;i<10;i++)
   {
    em.calcVI(50,200); 
    double amps = em.realPower;
    if ( amps < treshold )
    {
      return 0;
    }
    sum = amps + sum;
    delay(100);
   }
   // Moyenne
   avg=sum/10;
   if ( avg < treshold )
    {
      avg=0;
    }
    if ( last == 0)
     {
       last=avg; 
     }else
     {
       return avg;
     }
}

void changeMode()
{
 int val = digitalRead(BOOT_BUTTON);
 if ( val == 0 )
  {
    displayMode=displayMode+1;
    if ( loopMode == 1)
    {
      loopMode=0;
      displayMode=0;
    }
    
    
    Serial.println(" Display mode:" + String(displayMode));    
  }
 display.stopscroll();
 display.display();
 if ( displayMode > 4)
    {
      displayMode=0;
    }
 switch (displayMode) {
   case 0:
        displayWatt(String((int)watt1),String((int)watt2),String((int)watt3),String((int)watt4),String((int)watt5),String((int)watt6),String((int)watt7),String((int)watt8),String((int)watt9),String((int)watt10),String ((int)total));
        break;
   case 1:
        displayWifiConfiguration();
        break;
   case 2:
        displayConfiguration();
        break;
   case 3:
        if ( loopMode == 0)
        {
        screenOff();
        }
        break;
   case 4:
        display.clearDisplay();
        displayTitle("Auto Cycle in 1sec.");
        delay(1000);
        loopMode=1;
        displayWatt(String((int)watt1),String((int)watt2),String((int)watt3),String((int)watt4),String((int)watt5),String((int)watt6),String((int)watt7),String((int)watt8),String((int)watt9),String((int)watt10),String ((int)total));
        display.display();
        displayMode=0;
        break;
 }
 if ( loopMode == 1)
    {
      displayMode=displayMode + 1;
    }
}

void sendToInfluxDb()
{
  if ( sendUdp == 1 && pimpGrid.urlInfluxDb != "" )
  {
    // send the packet
    Serial.println("Sending UDP packet to influxDb");
    int A_IP=atoi(subStr(pimpGrid.urlInfluxDb, ".", 1));int B_IP=atoi(subStr(pimpGrid.urlInfluxDb, ".", 2));int C_IP=atoi(subStr(pimpGrid.urlInfluxDb, ".", 3));int D_IP=atoi(subStr(pimpGrid.urlInfluxDb, ".", 4));
    IPAddress ipInfluxdb(A_IP, B_IP, C_IP, D_IP);
    udp.beginPacket(ipInfluxdb, atoi(pimpGrid.portInfluxDb));
    String line = String("pimpMyGrid,deviceName=" +  String(pimpGrid.myName) +" watt1="+ String((int)watt1)+ ",watt2=" + String((int)watt2) + ",watt3=" + String((int)watt3) + ",watt4=" + String((int)watt4) + ",watt5=" + String((int)watt5) + ",watt6=" + String((int)watt6) + ",watt7=" + String((int)watt7) + ",watt8=" + String((int)watt8)+",watt9=" + String((int)watt9) + "watt10=" +String((int)watt10)+ ",total=" + String ((int)total));
    udp.print(line);
    udp.endPacket();
  }else
  {
    Serial.println("No network connection for influxDb");
  }
}

void loop()
{
 
 server.handleClient();
 delay(1);
 unsigned long currentMillis = millis();
 unsigned long currentMillisWifi = millis();
 if(currentMillis - previousMillis >= interval && ledValue == "blink" ) {
        previousMillis = currentMillis;
        if (ledState == LOW)
        {
          ledState = HIGH;  // Note that this switches the LED *off*
        }
        else
        {
          ledState = LOW;   // Note that this switches the LED *on*
        }
        digitalWrite(LED_BUILTIN, ledState);
   }
   if(currentMillisWifi - previousMillisWifi >= intervalWifi ) {
        previousMillisWifi = currentMillisWifi;
        
        // Wifi Test, still there ? mode Station only
       if ( strcmp(pimpGrid.aWifiMode, "ST")  == 0) 
        {
          Serial.println("    * Heartbeat (every minute) Checking wifi status, when mode station selected");
          if ( WiFi.waitForConnectResult() )
          {
            Serial.println("    * Heartbeat (every minute) Wifi station Ok, nothing to do.");  
          }else
          {
            Serial.println("    * Heartbeat (every minute) Wifi station seems to be bad, reconnect...");
            setStationWifi();
          }
        }
   }
 
 watt1 = compute(emon1);Serial.print("emon1: ");Serial.println(watt1);
 watt2 = compute(emon2);Serial.print("emon2: ");Serial.println(watt2);
 watt3 = compute(emon3);Serial.print("emon3: ");Serial.println(watt3);
 watt4 = compute(emon4);Serial.print("emon4: ");Serial.println(watt4);
 watt5 = compute(emon5);Serial.print("emon5: ");Serial.println(watt5);
 if ( extension == 1)
 {
  watt6 = compute(emon6);Serial.print("emon6: ");Serial.println(watt6);
  watt7 = compute(emon7);Serial.print("emon7: ");Serial.println(watt7);
  watt8 = compute(emon8);Serial.print("emon8: ");Serial.println(watt8);
  watt9 = compute(emon9);Serial.print("emon9: ");Serial.println(watt9);
  watt10 = compute(emon10);Serial.print("emon10: ");Serial.println(watt10);
 }
 total = watt1 + watt2 + watt3 + watt4 + watt5 + watt6 + watt7 + watt8 + watt9 + watt10 ;Serial.print("Total: ");Serial.println(total);
 
 
 changeMode();
 lastMeasurement = millis();
 sendToInfluxDb();
 
}


/**
 * Allow a substring from a String
 */
char* subStr (char* input_string, char *separator, int segment_number) {
 char *act, *sub, *ptr;
 static char copy[20];
 int i;
 strcpy(copy, input_string);
 for (i = 1, act = copy; i <= segment_number; i++, act = NULL) {
  sub = strtok_r(act, separator, &ptr);
  if (sub == NULL) break;
 }
 return sub;
}

#include <SPI.h>
#include <LoRa.h>

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

#include <OneWire.h>                
#include <DallasTemperature.h>
// Choose two Arduino pins to use for software serial
int RXPin = 3;
int TXPin = 4;  

TinyGPSPlus gps;

OneWire ourWire1(A1);
DallasTemperature sensor1(&ourWire1);

SoftwareSerial ss(RXPin, TXPin);

char tempera_1[8]={"\0"},peachim_1[8]={"\0"},gps_1[8]={"\0"}, gps_2[8]= {"\0"};
char *node_id = "<8379>";  
uint8_t datasend[36];
unsigned int count = 1; 
unsigned long new_time,old_time=0;
float gpsvalorlat,gpsvalorlng,peachimetro,temperatura,prumerPH,tem_1,tem_2;

const int pinPo = A0;

float RawValue=0;
float CorrectedValue=0;

void setup() 
{
  Serial.begin(9600);
   ss.begin(9600);
    
  //sensor1.begin();
  //sensor1.setResolution(11);

  lcd.init();
  lcd.backlight();
   
  while (!Serial);
  Serial.println("LoRa Sender");
   if (!LoRa.begin(505300000))   //868000000 is frequency
      { 
          Serial.println("Starting LoRa failed!");
          while (1);
      }
      // Setup Spreading Factor (6 ~ 12)
      LoRa.setSpreadingFactor(12);
      
      // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
      //Lower BandWidth for longer distance.
      LoRa.setSignalBandwidth(125000);
      
      // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
      LoRa.setCodingRate4(5);
      LoRa.setSyncWord(0x34); 
      Serial.println("LoRa init succeeded.");

      LoRa.onReceive(onReceive);   
      LoRa.receive();  

     
}

void temperaturaread()
{
  sensor1.requestTemperatures();
  RawValue = sensor1.getTempCByIndex(0);
  Serial.print("Sens. 1 ");
  //Serial.println(RawValue, 1);
  CorrectedValue=(RawValue*89)/98;
  Serial.println(CorrectedValue,1); 
  lcd.setCursor(0,0);
  lcd.print("T ");
  lcd.print(CorrectedValue);
  delay(900); 
}

void peachiread()
{
  int pole[10];
  int zaloha;
  unsigned long int prumerVysl = 0;
  
  for (int i = 0; i < 10; i++) {
    pole[i] = analogRead(pinPo);
    delay(10);
  }
  
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (pole[i] > pole[j]) {
        zaloha = pole[i];
        pole[i] = pole[j];
        pole[j] = zaloha;
      }
    }
  }
  
  for (int i = 2; i < 8; i++) {
    prumerVysl += pole[i];
  }
  
  prumerPH = (float)prumerVysl * (5.0 / 1024.0 / 6);
  Serial.print(prumerPH);
  peachimetro = -71.75 * prumerPH + 217.1;
  
  Serial.print("Namerene pH= ");
  //Serial.println(peachimetro);
  lcd.setCursor(8, 0);
  //lcd.print(prumerPH);
  lcd.print("PH ");
  lcd.print(peachimetro);
  
  delay(900);
}

void gpsread()
{
  
      Serial.print("Latitude= "); 
      Serial.print(gps.location.lat(), 6);
      gpsvalorlat=gps.location.lat();
      Serial.print(" Longitude= "); 
      Serial.println(gps.location.lng(), 6);
      gpsvalorlng=gps.location.lng();

      lcd.setCursor(0, 1);
      lcd.print("LT ");
      lcd.print(gpsvalorlat);
      lcd.print("LG ");
      lcd.print(gpsvalorlng);
      
    
}

void informacionwrite()
{
    char data[300] = "\0";
    for(int i = 0; i < 200; i++)
    {
       data[i] = node_id[i];
    }

    dtostrf(CorrectedValue,2,2,tempera_1);
    dtostrf(peachimetro,2,2,peachim_1);
    dtostrf(gpsvalorlat,3,5,gps_1);
    dtostrf(gpsvalorlng,3,5,gps_2);

    // Serial.println(tem_1);
     strcat(data,"field5=");
     strcat(data,tempera_1);
     strcat(data,"&field6=");
     strcat(data,peachim_1);
     strcat(data,"&field7=");
     strcat(data,gps_1);
     strcat(data,"&field8=");
     strcat(data,gps_2);
     strcpy((char *)datasend,data);
     
   //Serial.println((char *)datasend);
    //Serial.println(sizeof datasend);
      
}

void SendData()
{
     LoRa.beginPacket();
     LoRa.print((char *)datasend);
     LoRa.endPacket();
     Serial.println("Packet Sent");
}    
    

void loop() 
{
    new_time=millis();
    if (new_time - old_time >= 60 || old_time == 0)
    {
      while (ss.available() > 0 && new_time-old_time>=60)
      {
        gps.encode(ss.read());
        
        if (gps.location.isUpdated() && new_time-old_time>=60 )
        {
          old_time=new_time; 
          Serial.print("###########    ");
          Serial.print("COUNT=");
          Serial.print(count);
          Serial.println("    ###########");
          count++;
          temperaturaread();
          peachiread();
          gpsread();          
          informacionwrite();
          SendData();
          LoRa.receive();
               
        }
  
      }
    }
}

void onReceive(int packetSize) {
 
  // received a packet
  Serial.print("Received packet : ");

  // read packet
  for (int i = 0; i < packetSize; i++) {
      Serial.print((char)LoRa.read());
  }
  Serial.print("\n\r");  
}

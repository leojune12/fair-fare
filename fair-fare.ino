#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// GPS Pins
const int GPS_RX_PIN = 8;
const int GPS_TX_PIN = 9;

// GSM Pins
const int GSM_RX_PIN = 10;
const int GSM_TX_PIN = 11;

// RTC Pins
const int RTC_CLK_PIN = 5;
const int RTC_DAT_PIN = 6;
const int RTC_RST_PIN = 7;

String phone_number = "+639311235160";
String current_phone_number = "+639617789314";

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial SIM900(GSM_RX_PIN, GSM_TX_PIN);
virtuabotixRTC myRTC(RTC_CLK_PIN, RTC_DAT_PIN, RTC_RST_PIN);

const int BUTTON_PIN = 2;
const int HALL_SENSOR_PIN = 3;

long pulse_count = 0;
bool hall_sensor_last_value = false;

long int millis_start = 0;
int press_interval = 500;
int button_release_interval = 100;
int long_press_delay = 800;
bool button_first_press = false;

//FARE
const float min_distance_travelled = 1000;
const float min_FARE = 10;
const float succeeding_meter_rate = 0.005;
const float wheel_circumference = 1.90;   // Meters

float distance_travelled = 0;
float total_fare = 10;
float succeeding_distance = 0;

char incoming_char = 0;
String sms_content = "";

void setup() {
  // Set RTC current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //  myRTC.setDS1302Time(0, 36, 15, 4, 8, 3, 2023);

  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(HALL_SENSOR_PIN,INPUT);
  
  Serial.begin(19200);
  lcd.init();
  lcd.backlight();
  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(3, 2, "Getting  ready");
  Serial.println("FAIR FARE");
  Serial.println("Getting ready");

  SIM900.begin(115200);
  delay(500);

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);
  
  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(400);

  if(SIM900.available() > 0) {
    if (SIM900.read() > 0) {
      while (SIM900.available() > 0) {
        char c = SIM900.read();
      }
    }
  }

  gpsSerial.begin(9600);
  delay(1000);
  
  lcd_print(3, 1, "");
  lcd_print(3, 2, "");
  Serial.println("FAIR FARE Ready!");
}

void loop() {
  printDateTime();
  read_sms();
  odometer_loop();
//  read_gps();
}

void read_sms() {
  SIM900.listen();
  
  if(SIM900.available() > 0) {  
    //    while (SIM900.available() > 0) {
    //      incoming_char = SIM900.read();
    //      sms_content += String(incoming_char);  
    //      delay(10);    
    //    }
    //
    //    Serial.println(sms_content);
    //    
    //    if (sms_content.indexOf("Location") > 0 || sms_content.indexOf("location") > 0) {
    //      Serial.println("Location request received");
    //      Serial.println("Reading GPS...");
    //      lcd.setCursor(0, 1);
    //      lcd.print("  Location request  ");
    //      lcd.setCursor(0, 2);
    //      lcd.print("      received      ");
    //      lcd.setCursor(0, 3);
    //      lcd.print("   Reading GPS...   ");
    //      read_gps();
    //      sendSMS();
    //    }
    //
    //    incoming_char = 0;
    //    sms_content = "";
    if (SIM900.read() > 0) {
      while (SIM900.available() > 0) {
        char c = SIM900.read();
      }
      Serial.println("Location request received");
      Serial.println("Reading GPS...");
      lcd.setCursor(0, 1);
      lcd.print("  Location request  ");
      lcd.setCursor(0, 2);
      lcd.print("      received      ");
      lcd.setCursor(0, 3);
      lcd.print("   Reading GPS...   ");
      read_gps();
      sendSMS();
    }
  }
}

void odometer_loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(long_press_delay);
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Halong!");
      lcd_print(0, 1, "");
      lcd_print(6, 2, "Halong!");
      lcd_print(0, 3, "");
      while(true) {
        if (millis() - millis_start > button_release_interval) {
          if (digitalRead(BUTTON_PIN) == HIGH) {
            delay(2000);
            start_fare_solving();
            break;
          }
        }
      }
    }
  }
}

void start_fare_solving() {
  pulse_count = 0;
  distance_travelled = 0;
  total_fare = 10;
  succeeding_distance = 0;

//  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(0, 1, "");
  lcd_print(0, 2, "Distance: " + String(distance_travelled) + "Km");
  lcd_print(0, 3, "Fare: Php" + String(total_fare));
  
  while(true)  {
    int sensor_value = digitalRead(HALL_SENSOR_PIN);
    int break_status = false;
    
    if(sensor_value == HIGH)  {
      hall_sensor_last_value = false;
    }

    if (hall_sensor_last_value == false)  {
      while (true) {
        sensor_value = digitalRead(HALL_SENSOR_PIN);
        if (sensor_value == LOW) {
          pulse_count++;
          calculate_fare(pulse_count);
          lcd.setCursor(10, 2);
          lcd.print(String(distance_travelled) + "Km");
          lcd.setCursor(6, 3);
          lcd.print("Php" + String(total_fare));
          Serial.print("Distance(Km): ");
          Serial.print(String(distance_travelled));
          Serial.print(" FARE(Php): ");
          Serial.println(String(total_fare));
          hall_sensor_last_value = true;
          break;
        }
        if(digitalRead(BUTTON_PIN) == LOW)  {
          break_status = true;
          break;
        }
      }
    }

    if (break_status || digitalRead(BUTTON_PIN) == LOW) {
      delay(long_press_delay);
      if (digitalRead(BUTTON_PIN) == LOW) {
        
        lcd_print(0, 1, "");
        lcd_print(5, 2, "Thank you!");
        lcd_print(0, 3, "");
        Serial.println("Thank you!");
        delay(2000);
        lcd_print(0, 2, "");
        bool end_fare_solving = false;
        while(true) {
          if (millis() - millis_start > button_release_interval) {
            if (digitalRead(BUTTON_PIN) == HIGH) {
              end_fare_solving = true;
              break;
            }
          }
        }
        if (end_fare_solving) {
          break;
        }
      }
    }
  }
}

void calculate_fare(long total_pulse) {
  distance_travelled = (wheel_circumference * total_pulse) / 1000;
  if ((distance_travelled * 1000) > min_distance_travelled) {
    succeeding_distance = (distance_travelled * 1000) - min_distance_travelled;
    total_fare = (succeeding_distance * 0.005) + 10; 
  } else {
    total_fare = 10;
  }
}

void lcd_print(int vertical_location, int horizontal_location, String text) {
  lcd.setCursor(0, horizontal_location);
  lcd.print("                    ");
  lcd.setCursor(vertical_location, horizontal_location);
  lcd.print(text);
}

void read_gps() {

  gpsSerial.listen();
  bool break_flag = false;
  
  while (true) {
    while (gpsSerial.available()) {
      if (gps.encode(gpsSerial.read())) {
        displayGpsInfo();      
        break_flag = true;
        break;
      }
    }

    if (break_flag) {
      break;
    }
  }
}

void displayGpsInfo() {
  if (gps.location.isValid()) {
    
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
//    lcd.setCursor(0, 2);
//    lcd.print("Lat: " + String(gps.location.lat()));
//    lcd.setCursor(0, 3);
//    lcd.print("Long: " + String(gps.location.lng()));
  }
  else {
    
    Serial.println("Location: Not Available");
//    lcd.setCursor(0, 2);
//    lcd.print("Location:");
//    lcd.setCursor(0, 3);
//    lcd.print("Not Available");
  }
}

void printDateTime() {
  
  myRTC.updateTime();
  
  Serial.print("Current Date / Time: ");
  Serial.print(myRTC.dayofmonth);
  Serial.print("/");
  Serial.print(myRTC.month);
  Serial.print("/");
  Serial.print(myRTC.year);
  Serial.print(" ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.println(myRTC.seconds);

  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("   Date: " + 
    (myRTC.month < 10 ? "0" + String(myRTC.month) : String(myRTC.month)) + "-" + 
    (myRTC.dayofmonth < 10 ? "0" + String(myRTC.dayofmonth) : String(myRTC.dayofmonth)) + "-" + 
    String(myRTC.year).substring(2));
  lcd.setCursor(0, 3);
  lcd.print("   Time: " + 
    (myRTC.hours < 10 ? "0" + String(myRTC.hours) : String(myRTC.hours)) + ":" + 
    (myRTC.minutes < 10 ? "0" + String(myRTC.minutes) : String(myRTC.minutes)) + ":" + 
    (myRTC.seconds < 10 ? "0" + String(myRTC.seconds) : String(myRTC.seconds)));
}

void sendSMS() {

  Serial.println("Sending location...");
  lcd.setCursor(0, 3);
  lcd.print("Sending location... ");

  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT + CMGS = \"" + phone_number + "\"");
  delay(100);

  if (gps.location.isValid()) {
    SIM900.println("Location: http://maps.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6) + "&ll=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6) + "&z=17");
  } else {
    SIM900.println("Location: Not available. Please try again later.");
  }
  
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26);
  delay(100);
  SIM900.println();

  delay(3000);

  Serial.println("Location Sent");
  lcd.setCursor(0, 3);
  lcd.print("   Location Sent    ");

  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  
  delay(3000);
}

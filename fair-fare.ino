#include <TimeLib.h>
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
float total_FARE = 10;
float succeeding_distance = 0;

char Time[]  = "Time: 00:00:00";
char Date[]  = "Date: 00-00-00";
byte last_second, Second, Minute, Hour, Day, Month;
int Year;

#define time_offset 28800  // define a clock offset of 3600 seconds (1 hour) ==> UTC + 1

void setup() {
  
  Serial.begin(19200);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(HALL_SENSOR_PIN,INPUT);

  lcd.init();
  lcd.backlight();
  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(3, 2, "Getting  ready");
  Serial.println("FAIR FARE");
  Serial.println("Getting ready");

  gpsSerial.begin(9600);
  delay(1000);

  SIM900.begin(115200);
  delay(1000);
  
//  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(3, 1, "");
  lcd_print(3, 2, "");
  Serial.println("FAIR FARE Ready!");
}

void loop() {
//  printDateTime();
  odometer_loop();
  read_gps();
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
            start_FARE_solving();
            break;
          }
        }
      }
    }
  }
}

void start_FARE_solving() {
  pulse_count = 0;
  distance_travelled = 0;
  total_FARE = 10;
  succeeding_distance = 0;

//  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(0, 1, "");
  lcd_print(0, 2, "Distance: " + String(distance_travelled) + "Km");
  lcd_print(0, 3, "FARE: Php" + String(total_FARE));
  
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
          calculate_FARE(pulse_count);
          lcd.setCursor(10, 2);
          lcd.print(String(distance_travelled) + "Km");
          lcd.setCursor(6, 3);
          lcd.print("Php" + String(total_FARE));
          Serial.print("Distance(Km): ");
          Serial.print(String(distance_travelled));
          Serial.print(" FARE(Php): ");
          Serial.println(String(total_FARE));
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
        bool end_FARE_solving = false;
        while(true) {
          if (millis() - millis_start > button_release_interval) {
            if (digitalRead(BUTTON_PIN) == HIGH) {
              end_FARE_solving = true;
              break;
            }
          }
        }
        if (end_FARE_solving) {
          break;
        }
      }
    }
  }
}

void calculate_FARE(long total_pulse) {
  distance_travelled = (wheel_circumference * total_pulse) / 1000;
  if ((distance_travelled * 1000) > min_distance_travelled) {
    succeeding_distance = (distance_travelled * 1000) - min_distance_travelled;
    total_FARE = (succeeding_distance * 0.005) + 10; 
  } else {
    total_FARE = 10;
  }
}

void lcd_print(int vertical_location, int horizontal_location, String text) {
  lcd.setCursor(0, horizontal_location);
  lcd.print("                    ");
  lcd.setCursor(vertical_location, horizontal_location);
  lcd.print(text);
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

  lcd.setCursor(3, 2);
  lcd.print("Date: " + 
    (myRTC.month < 10 ? "0" + String(myRTC.month) : String(myRTC.month)) + "-" + 
    (myRTC.dayofmonth < 10 ? "0" + String(myRTC.dayofmonth) : String(myRTC.dayofmonth)) + "-" + 
    String(myRTC.year).substring(2));
  lcd.setCursor(3, 3);
  lcd.print("Time: " + 
    (myRTC.hours < 10 ? "0" + String(myRTC.hours) : String(myRTC.hours)) + ":" + 
    (myRTC.minutes < 10 ? "0" + String(myRTC.minutes) : String(myRTC.minutes)) + ":" + 
    (myRTC.seconds < 10 ? "0" + String(myRTC.seconds) : String(myRTC.seconds)));
}

void print_date_time_from_gps() {

  // get time from GPS module
  if (gps.time.isValid())
  {
    Minute = gps.time.minute();
    Second = gps.time.second();
    Hour   = gps.time.hour();
  }

  // get date drom GPS module
  if (gps.date.isValid())
  {
    Day   = gps.date.day();
    Month = gps.date.month();
    Year  = gps.date.year();
  }

  if(last_second != gps.time.second())  // if time has changed
  {
    last_second = gps.time.second();

    // set current UTC time
    setTime(Hour, Minute, Second, Day, Month, Year);
    // add the offset to get local time
    adjustTime(time_offset);

    // update time array
    Time[12] = second() / 10 + '0';
    Time[13] = second() % 10 + '0';
    Time[9]  = minute() / 10 + '0';
    Time[10] = minute() % 10 + '0';
    Time[6]  = hour()   / 10 + '0';
    Time[7]  = hour()   % 10 + '0';

    // update date array
    Date[12] = (year()  / 10) % 10 + '0';
    Date[13] =  year()  % 10 + '0';
    Date[6]  =  month() / 10 + '0';
    Date[7] =  month() % 10 + '0';
    Date[9]  =  day()   / 10 + '0';
    Date[10]  =  day()   % 10 + '0';

    // print time & date
    print_wday(weekday());   // print day of the week
    lcd.setCursor(3, 2);     // move cursor to column 0 row 2
    lcd.print(Date);         // print time (HH:MM:SS)
    lcd.setCursor(3, 3);     // move cursor to column 0 row 3
    lcd.print(Time);         // print date (DD-MM-YYYY)
  }
  
  Serial.print(Date);
  Serial.print(" - ");
  Serial.println(Time);
}

// function for displaying day of the week
void print_wday(byte wday)
{
  lcd.setCursor(5, 1);  // move cursor to column 5, row 1
  switch(wday)
  {
    case 1:  lcd.print("  Sunday  ");   break;
    case 2:  lcd.print("  Monday  ");   break;
    case 3:  lcd.print(" Tuesday ");   break;
    case 4:  lcd.print("Wednesday");   break;
    case 5:  lcd.print("Thursday ");   break;
    case 6:  lcd.print("  Friday  ");   break;
    default: lcd.print("Saturday ");
  }

}

void read_gps() {

  gpsSerial.listen();

  bool break_flag = false;

  while (true) {

    while (gpsSerial.available()) {
    
      if (gps.encode(gpsSerial.read())) {
        
        displayGpsInfo();
        print_date_time_from_gps();
        
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

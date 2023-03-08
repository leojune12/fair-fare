#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// RTC Pins
const int RTC_CLK_PIN = 5;
const int RTC_DAT_PIN = 6;
const int RTC_RST_PIN = 7;

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

//Fare
const float min_distance_travelled = 1000;
const float min_fare = 10;
const float succeeding_meter_rate = 0.005;
const float wheel_circumference = 1.90;   // Meters

float distance_travelled = 0;
float total_fare = 10;
float succeeding_distance = 0;

void setup() {
  // Set RTC current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //  myRTC.setDS1302Time(0, 36, 15, 4, 8, 3, 2023);
  
  Serial.begin(9600);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(HALL_SENSOR_PIN,INPUT);
  
  lcd.init();
  lcd.backlight();
  lcd_print(5, 1, "Fair  Fare");
  lcd_print(3, 2, "Getting  ready");
  delay(2000);
  lcd_print(5, 0, "Fair  Fare");
  lcd_print(3, 1, "");
  lcd_print(3, 2, "");
  Serial.println("Fair Fare");
  Serial.println("Getting ready");
  Serial.println("Fair Fare sReady!");
}

void loop()  {
  printDateTime();
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(long_press_delay);
    if (digitalRead(BUTTON_PIN) == LOW) {
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

  lcd_print(5, 0, "Fair  Fare");
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
          Serial.print(" Fare(Php): ");
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
        
        lcd_print(5, 0, "Fair  Fare");
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

void printDateTime() {
  
  myRTC.updateTime();

  int a = 10;
  
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
  lcd.print("Date: " + (myRTC.month < 10 ? "0" + String(myRTC.month) : String(myRTC.month)) + "/" + (myRTC.dayofmonth < 10 ? "0" + String(myRTC.dayofmonth) : String(myRTC.dayofmonth)) + "/" + String(myRTC.year).substring(2));
  lcd.setCursor(3, 3);
  lcd.print("Time: " + (myRTC.hours < 10 ? "0" + String(myRTC.hours) : String(myRTC.hours)) + ":" + (myRTC.minutes < 10 ? "0" + String(myRTC.minutes) : String(myRTC.minutes)) + ":" + (myRTC.seconds < 10 ? "0" + String(myRTC.seconds) : String(myRTC.seconds)));
}

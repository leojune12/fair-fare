#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// GPS Pins
const int GPS_RX_PIN = 8;
const int GPS_TX_PIN = 9;

// GSM Pins
const int GSM_RX_PIN = 10;
const int GSM_TX_PIN = 11;

String phone_number = "+639XXXXXXXXX";
String current_phone_number = "+639XXXXXXXXX";

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial SIM900(GSM_RX_PIN, GSM_TX_PIN);

const int BUTTON_PIN_1 = 2;
const int BUTTON_PIN_2 = 3;
const int HALL_SENSOR_PIN = 4;


bool hall_sensor_last_value = false;
long int millis_start = 0;
int button_release_interval = 100;
int long_press_delay = 800;

//FARE
const int min_distance_travelled = 1000; // Meters
const int min_fare = 10;
const float succeeding_meter_rate = 0.008;
const float wheel_circumference = 1.75552;   // Meters

// Passenger 1
bool passenger_1_status = false;
long pulse_count_1 = 0;
float distance_travelled_1 = 0;
float total_fare_1 = 10;
float succeeding_distance_1 = 0;

// Passenger 2
bool passenger_2_status = false;
long pulse_count_2 = 0;
float distance_travelled_2 = 0;
float total_fare_2 = 10;
float succeeding_distance_2 = 0;

void setup() {
  pinMode(BUTTON_PIN_1,INPUT_PULLUP);
  pinMode(BUTTON_PIN_2,INPUT_PULLUP);
  pinMode(HALL_SENSOR_PIN,INPUT);
  
  Serial.begin(19200);
  lcd.init();
  lcd.backlight();
  lcd_print(5, 0, "FAIR  FARE");
  lcd_print(0, 1, "-------------------");
  lcd_print(3, 2, "Getting  Ready");
  Serial.println("FAIR FARE");
  Serial.println("Getting ready");

  SIM900.begin(115200);
  delay(500);

  Serial.println("Initializing GSM");

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(500);

  if(SIM900.available() > 0) {
    String sms_content = SIM900.readString();
//    Serial.println(sms_content);
  }
  
  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(500);

  if(SIM900.available() > 0) {
    String sms_content = SIM900.readString();
//    Serial.println(sms_content);
  }

  gpsSerial.begin(9600);
  delay(1000);
  
  lcd_print(0, 1, "--------------------");
  lcd_print(0, 2, "       Vacant      ");
  Serial.println("FAIR FARE Ready!");
}

void loop() {
//  printDateTime();
  read_sms();
  odometer_loop();
//  read_gps();
}

void read_sms() {
  SIM900.listen();
  if(SIM900.available() > 0) {
    String sms_content = SIM900.readString();
    if (sms_content.length() > 30) {
      read_gps();
      sendSMS();
    }
  }
}

void odometer_loop() {
  if (digitalRead(BUTTON_PIN_1) == LOW) {
    delay(long_press_delay);
    if (digitalRead(BUTTON_PIN_1) == LOW) {
      lcd_print(6, 2, "Halong!");
      lcd_print(0, 3, "");
      while(true) {
        if (digitalRead(BUTTON_PIN_1) == HIGH) {
          delay(2000);
          start_fare_solving();
          break;
        }
      }
    }
  }
}

void start_fare_solving() {
  passenger_1_status = true;
  pulse_count_1 = 0;
  distance_travelled_1 = 0;
  total_fare_1 = 10;
  succeeding_distance_1 = 0;

  pulse_count_2 = 0;
  distance_travelled_2 = 0;
  total_fare_2 = 10;
  succeeding_distance_2 = 0;

  lcd_print(0, 0, "P1 Distance: " + String(distance_travelled_1) + "Km");
  lcd_print(0, 1, "P1 Fare: Php" + String(total_fare_1));
  lcd_print(0, 2, "P2 Distance: --     ");
  lcd_print(0, 3, "P2 Fare: --         ");
  
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
          if (passenger_1_status){
            pulse_count_1++;
            calculate_fare_1(pulse_count_1);
            lcd.setCursor(13, 0);
            lcd.print(String(distance_travelled_1) + "Km");
            lcd.setCursor(9, 1);
            lcd.print("Php" + String(total_fare_1));
          } else {
            pulse_count_1 = 0;
            distance_travelled_1 = 0;
            total_fare_1 = 10;
            succeeding_distance_1 = 0;
          }
          if (passenger_2_status){
            pulse_count_2++;
            calculate_fare_2(pulse_count_2);
            lcd.setCursor(13, 2);
            lcd.print(String(distance_travelled_2) + "Km");
            lcd.setCursor(9, 3);
            lcd.print("Php" + String(total_fare_2));
          } else {
            pulse_count_2 = 0;
            distance_travelled_2 = 0;
            total_fare_2 = 10;
            succeeding_distance_2 = 0;
          }
          
          hall_sensor_last_value = true;
          break;
        }
        
        if(digitalRead(BUTTON_PIN_1) == LOW)  {
          delay(long_press_delay);
          if (digitalRead(BUTTON_PIN_1) == LOW) {
            if (passenger_1_status) {
              passenger_1_status = false;
              pulse_count_1 = 0;
              distance_travelled_1 = 0;
              total_fare_1 = 10;
              succeeding_distance_1 = 0;
              lcd.setCursor(13, 0);
              lcd.print("--     ");
              lcd.setCursor(9, 1);
              lcd.print("--         ");             
            } else {
              passenger_1_status = true;
              lcd.setCursor(13, 0);
              lcd.print(String(distance_travelled_1) + "Km");
              lcd.setCursor(9, 1);
              lcd.print("Php" + String(total_fare_1));
            }
            while(true) {
              if (digitalRead(BUTTON_PIN_1) == HIGH) {
                break;
              }
            }
          }
        }

        if(digitalRead(BUTTON_PIN_2) == LOW)  {
          delay(long_press_delay);
          if (digitalRead(BUTTON_PIN_2) == LOW) {
            if (passenger_2_status) {
              passenger_2_status = false;
              pulse_count_2 = 0;
              distance_travelled_2 = 0;
              total_fare_2 = 10;
              succeeding_distance_2 = 0;
              lcd.setCursor(13, 2);
              lcd.print("--     ");
              lcd.setCursor(9, 3);
              lcd.print("--         ");               
            } else {
              passenger_2_status = true;
              lcd.setCursor(13, 2);
              lcd.print(String(distance_travelled_2) + "Km");
              lcd.setCursor(9, 3);
              lcd.print("Php" + String(total_fare_2));
            }
            while(true) {
              if (digitalRead(BUTTON_PIN_2) == HIGH) {
                break;
              }
            }
          }
        }

        if (!passenger_2_status && !passenger_1_status) {
          break_status = true;
          break;
        }
      }
    }

    if (break_status) {
      lcd_print(0, 0, "     FAIR  FARE     ");
      lcd_print(0, 1, "--------------------");
      lcd_print(0, 2, "     Thank you!");
      lcd_print(0, 3, "");
      Serial.println("Thank you!");
      delay(2000);
      lcd_print(0, 2, "       Vacant      ");
      break;
    }
  }
}

void calculate_fare_1(long total_pulse) {
  distance_travelled_1 = (wheel_circumference * total_pulse) / 1000;
  if ((distance_travelled_1 * 1000) > min_distance_travelled) {
    succeeding_distance_1 = (distance_travelled_1 * 1000) - min_distance_travelled;
    total_fare_1 = (succeeding_distance_1 * 0.005) + 10; 
  } else {
    total_fare_1 = min_fare;
  }
}

void calculate_fare_2(long total_pulse) {
  distance_travelled_2 = (wheel_circumference * total_pulse) / 1000;
  if ((distance_travelled_2 * 1000) > min_distance_travelled) {
    succeeding_distance_2 = (distance_travelled_2 * 1000) - min_distance_travelled;
    total_fare_2 = (succeeding_distance_2 * 0.005) + 10; 
  } else {
    total_fare_2 = min_fare;
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
//    Serial.print("Latitude: ");
//    Serial.println(gps.location.lat(), 6);
//    Serial.print("Longitude: ");
//    Serial.println(gps.location.lng(), 6);
  }
  else {
//    Serial.println("Location: Not Available");
  }
}

void sendSMS() {

  Serial.println("Sending location...");

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r");
  delay(100);

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

  // Set module to send SMS data to serial out upon receipt
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(500);
  
  delay(2500);
}

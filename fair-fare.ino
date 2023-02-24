#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

const int BUTTON_PIN = 2;
const int HALL_SENSOR_PIN = 3;

int pulse_count = 0;
bool hall_sensor_last_value = false;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(HALL_SENSOR_PIN,INPUT);
  
  lcd.init();
  lcd.backlight();
  lcd_print(5, 1, "Fair Fare");
  delay(2000);

  
  lcd_print(0, 0, "");
  lcd_print(2, 1, "Press the button");
  lcd_print(6, 2, "to start");
}

void loop()  {
  if(digitalRead(BUTTON_PIN) == LOW)  {
    start_fare_solving();
  }
}

void start_fare_solving() {
  pulse_count = 0;
  lcd_print(0, 0, "Total pulse count:");
  lcd_print(0, 1, String(pulse_count));
  lcd_print(0, 2, "");
  delay(1000);
  
  while(1)  {
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
          lcd_print(0, 1, String(pulse_count));
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
      break;
    }
  }
  
  lcd_print(0, 0, "");
  lcd_print(2,1, "Press the button");
  lcd_print(6,2, "to start");

  delay(1000);
}

void lcd_print(int vertical_location, int horizontal_location, String text) {
  lcd.setCursor(0, horizontal_location);
  lcd.print("                    ");
  lcd.setCursor(vertical_location, horizontal_location);
  lcd.print(text);
}

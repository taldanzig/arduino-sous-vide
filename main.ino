#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
int rs = 8;
int e = 9;

float target_temp = 30;
float temp_inc = 0.5;

int output_pin = 3;

boolean heater_state;
float min_temp_change = 0.5;

// LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(rs, e, 4, 5, 6, 7);

void setup()
{
  //analogReference(DEFAULT);
  analogReference(EXTERNAL);
  lcd.begin(16,2);
  lcd.print("Temp: ");
  pinMode(output_pin, OUTPUT);
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(500);
}

int less_than(const void* pa, const void* pb)
{
  return *(const int*)pa - *(const int*)pb;
}

void loop()
{
  lcd.setCursor(0, 1);
  lcd.print(target_temp,1);
  lcd.print("     ");

  const unsigned T_JIFFY = 10; // ms
  const unsigned T_BUTTON = 250 / T_JIFFY; // ms/ms
  static int t_button;
  if (t_button == 0) {
    int buttonInput = analogRead(A0);
    if (buttonInput > 250 && buttonInput < 900) {
      target_temp -= temp_inc;
      t_button = T_BUTTON;
    } else if (buttonInput > 100 && buttonInput < 900) {
      target_temp += temp_inc;
      t_button = T_BUTTON;
    }
  } else
    t_button--;

  const float conversion_factor = 382.0/1024;
  const unsigned T_ANALOG = 10 / T_JIFFY; // ms/ms
  const unsigned N_READINGS = 500;
  const unsigned N_TRIM = 100;

  static boolean blinky;
  static unsigned raw_temp = 1024;
  static int t_analog;
  if (--t_analog <= 0) {
    t_analog = T_ANALOG;

    static unsigned n_readings;
    static unsigned readings[N_READINGS];
    readings[n_readings++] = analogRead(A1);
    if (n_readings >= N_READINGS) {
      qsort(readings, n_readings, sizeof readings[0], less_than);
      long unsigned sum = 0;
      for (int i = N_TRIM; i < n_readings - N_TRIM; ++i)
        sum += readings[i];
      raw_temp = sum / (n_readings - 2 * N_TRIM);
      n_readings = 0;
      blinky = !blinky;
    }
  }
  //xxx delay(T_JIFFY);

  float temp = raw_temp * conversion_factor - 273;
  temp = temp * 2;
  temp = round(temp);
  temp = temp / 2;
  lcd.print(temp, 1);
  lcd.print("  ");
  lcd.print(blinky ? '.' : ' ');

  float temp_diff = temp - target_temp;

  if (temp_diff > min_temp_change) {
    // Too hot
    heater_state = false;
  } else if (temp_diff < -min_temp_change) {
    // Too cold
    heater_state = true;
  }

  lcd.setCursor(10, 0);

  static boolean relay_state;
  static int t_relay;
  if (heater_state) {
    if (--t_relay < 0) {
      const unsigned T_RELAY = 5000 / T_JIFFY; // ms/ms
      t_relay = T_RELAY;
      relay_state = !relay_state;
    }
  } else {
    relay_state = false;
    t_relay = 0;
  }
  digitalWrite(output_pin, relay_state ? HIGH : LOW);

  lcd.write(heater_state ? (relay_state ? "ON  " : "WAIT") : "OFF ");
}


//======================================================================================================================
// SolarCalculator Library for Arduino example sketch: SunriseSunset.ino
//  
// Calculate the times of sunrise, solar noon and sunset for a given date and location.
//
// Tested with Arduino IDE 1.8.19 and Arduino Uno
//======================================================================================================================

#include <SolarCalculator.h>

void setup()
{
  Serial.begin(9600);

  // Date
  int year = 2022;
  int month = 7;
  int day = 31;

  // Location
  double latitude = 47.54;
  double longitude = -122.636;
  int utc_offset = -8;

  double transit, sunrise, sunset;

  // Calculate the times of sunrise, transit and sunset, in hours (UTC)
  //calcSunriseSunset(year, month, day, latitude, longitude, transit, sunrise, sunset);

  // Get the approximate times (minimum program size) (iterations = 0)
  calcSunriseSunset(year, month, day, latitude, longitude, transit, sunrise, sunset, SUNRISESET_STD_ALTITUDE, 0);

  // Print results
  char str[6];
  Serial.print("==>");
  Serial.println(hoursToString(sunrise + utc_offset, str));
  Serial.print("==>");
  Serial.println(hoursToString(transit + utc_offset, str));
  Serial.print("==>");
  Serial.println(hoursToString(sunset + utc_offset, str));
}

void loop()
{
}

// Rounded HH:mm format
char * hoursToString(double h, char *str)
{
  int m = int(round(h * 60));
  int hr = (m / 60) % 24;
  int mn = m % 60;

  Serial.print("--");
  Serial.println(h);

  str[0] = (hr / 10) % 10 + '0';
  str[1] = (hr % 10) + '0';
  str[2] = ':';
  str[3] = (mn / 10) % 10 + '0';
  str[4] = (mn % 10) + '0';
  str[5] = '\0';
  return str;
}

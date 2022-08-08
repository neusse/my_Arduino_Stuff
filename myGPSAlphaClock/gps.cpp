/*
 * GPS support for Alpha Clock Five
 * (C) 2013 William B Phelps
 * All commercial rights reserved
 */

// Explination of not waiting for a GPS fix to set time:
//
// Each satellite broadcasts time. Only one is needed to determine the time with accuracy to distance 
// to the satellite in light seconds - under 0.088s of error.
//
// But you need at least three, preferably more satellites to get the location fix and the 
// high-precision time (adjusted for distance from the satellite = time it takes for the signal to 
// reach you.) As your GPS obtains more signals, it calculates its position (fix) and adjusts the clock, 
// but the initial first broadcast is enough to establish timestamps on messages.


#include "gps.h"
#include "alphafive.h"
// this should be changed over to use the DS3231 
#include "DS1307RTC.h" // For optional RTC module. (This library included with the Arduino Time library)

String split_arr[12];

//#define gpsTimeoutLimit 5 // 5 seconds until we display the "no gps" message

unsigned long tGPSupdateUT = 0; // time since last GPS update in UT
unsigned long tGPSupdate = 0;   // time since last GPS update in local time
byte GPSupdating = false;

int8_t TZ_hour = -8;
int8_t TZ_minutes = 0;
// uint8_t DST_offset = 0;

#define GPSBUFFERSIZE 128 // plenty big
char gpsBuffer[GPSBUFFERSIZE];


//#############################################################################
// xsplit - split a string into an array of strings at a given delimiter ","
//#############################################################################
void xsplit (String mystr) {
  int i = 0;
  int a = mystr.indexOf(",");
  while (a != -1) {
    split_arr[i] = mystr.substring(0, a);
    if (a+1 > mystr.length()-1) {
      break;
    }
    mystr = mystr.substring(a+1);
    a = mystr.indexOf(",");
    i++;
    if ( i >= 10) {
        break;
    }
  }
  if (mystr.length() != 0) {
    split_arr[i] = mystr;
  }
}

//#############################################################################
// parse the GPS data and update the clock
//#############################################################################
uint32_t parsedecimal(char *str)
{
    uint32_t d = 0;

    while (str[0] != 0)
    {
        if ((str[0] > '9') || (str[0] < '0'))
            return d; // no more digits

        d = (d * 10) + (str[0] - '0');
        str++;
    }
    return d;
}

//#############################################################################
// get data from gps and update the clock (if possible)
//#############################################################################
void getGPSdata(void)
{
    //  char charReceived = UDR0;  // get a byte from the port
    char charReceived = Serial1.read();
    uint8_t bufflen = strlen(gpsBuffer);

    // If the buffer has not been started, check for '$'
    if ((bufflen == 0) && ('$' != charReceived))
        return; // wait for start of next sentence from GPS

    if (bufflen < (GPSBUFFERSIZE - 1))
    { // is there room left? (allow room for null term)

        if ('\r' != charReceived)
        {                                         // end of sentence?
            strncat(gpsBuffer, &charReceived, 1); // add char to buffer
            return;
        }

        strncat(gpsBuffer, "*", 1); // mark end of buffer just in case

        if (strncmp(gpsBuffer, "$GPRMC", 6) == 0)
        {
            Serial.println(gpsBuffer);
            parseGPSdata(gpsBuffer); // check for GPRMC sentence and set clock
        }
    } // if space left in buffer
    // either buffer is full, or the message has been processed. reset buffer for next message
    memset(gpsBuffer, 0, GPSBUFFERSIZE); // clear GPS buffer
} // getGPSdata

// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | Field | Structure  | Field Description                                                                         | Symbol    | Example       |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 1     | $GPRMC     | Log header. For information about the log headers, see ASCII, Abbreviated ASCII or Binary.|           | $GPRMC        |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 2     | utc        | UTC of position                                                                           | hhmmss.ss | 144326        |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 3     | pos status | Position status (A = data valid, V = data invalid)                                        | A         | A             |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 4     | lat        | Latitude (DDmm.mm)                                                                        | llll.ll   | 5107.0017737  |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 5     | lat dir    | Latitude direction: (N = North, S = South)                                                | a         | N             |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 6     | lon        | Longitude (DDDmm.mm)                                                                      | yyyyy.yy  | 11402.3291611 |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 7     | lon dir    | Longitude direction: (E = East, W = West)                                                 | a         | W             |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 8     | speed Kn   | Speed over ground, knots                                                                  | x.x       | 0.08          |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 9     | track true | Track made good, degrees True                                                             | x.x       | 323.3         |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 10    | date       | Date: dd/mm/yy                                                                            | xxxxxx    | 210307        |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 11    | mag var    | Magnetic variation, degrees                                                               | x.x       | 0             |
// |       |            | Note that this field is the actual magnetic variation and will always be positive.        |           |               |
// |       |            | The direction of the magnetic variation is always positive.                               |           |               |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 12    | var dir    | Magnetic variation direction E/W                                                          | a         | E             |
// |       |            | Easterly variation (E) subtracts from True course.                                        |           |               |
// |       |            | Westerly variation (W) adds to True course.                                               |           |               |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 13    | mode ind   | Positioning system mode indicator, see Table: NMEA Positioning System Mode Indicator      | a         | A             |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 14    | *xx        | Check sum                                                                                 | *hh       | *20           |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+
// | 15    | [CR][LF]   | Sentence terminator                                                                       |           | [CR][LF]      |
// +-------+------------+-------------------------------------------------------------------------------------------+-----------+---------------+

//#############################################################################
// parseGPSdata
//#############################################################################
void parseGPSdata(char *gpsBuffer) {
  time_t tNow, tDelta;
  tmElements_t tm;
  uint32_t tmp;
  char myTime[10];
  char myDate[10];
  
  xsplit(gpsBuffer);
  // convert string object to char Array so we can use it.
  split_arr[1].toCharArray(myTime, 10);
  split_arr[9].toCharArray(myDate, 10);
        
  for (int i = 0; i < 10; i++) {
//    Serial.println(split_arr[i]); // we can see all the eliments of the $GPRMC record without using a walking strtok
    split_arr[i] = "";
  }
  Serial.println(myTime);
  Serial.println(myDate);
      
  // since we dont need a location fix we can use the datetime date without checking.  only need 1 sattleite.
  tmp = parsedecimal(myTime); // parse integer portion
  tm.Hour = tmp / 10000;
  tm.Minute = (tmp / 100) % 100;
  tm.Second = tmp % 100;
  
  tmp = parsedecimal(myDate);
  tm.Day = tmp / 10000;
  tm.Month = (tmp / 100) % 100;
  tm.Year = tmp % 100;
  
  tm.Year = y2kYearToTm(tm.Year); // convert yy year to (yyyy-1970) (add 30)
  tNow = makeTime(tm);            // convert to time_t
  tDelta = abs(tNow - tGPSupdateUT);

  if ((tGPSupdateUT > 0) && (tDelta > SECS_PER_DAY)) {
      // GPS time jumped more than 1 day
      Serial.println("GPS error");
      //a5tone(2093, 200);
      strcpy(gpsBuffer, ""); // wipe GPS buffer
      return;
  }
  GPSupdating = false; // valid GPS data received, flip the LED off

  if ((tm.Second == 59) || (tDelta >= 60)) {
    // update RTC once/minute or if it's been 60 seconds
    // beep(1000, 1);  // debugging
    a5loadOSB_DP("____2", a5_brightLevel); // wbp
    a5BeginFadeToOSB();
    GPSupdating = true;
    tGPSupdateUT = tNow;                                        // remember time of this update (UT)
    tNow = tNow + (long)(TZ_hour + DST_offset) * SECS_PER_HOUR; // add time zone hour offset & DST offset
  
    if (TZ_hour < 0)                                   // add or subtract time zone minute offset
        tNow = tNow - (long)TZ_minutes * SECS_PER_MIN; // 01feb13/wbp
    else
        tNow = tNow + (long)TZ_minutes * SECS_PER_MIN; // 01feb13/wbp
  
    setTime(tNow);
  
    if (UseRTC)
        RTC.set(now()); // set RTC from adjusted GPS time & date
  
    tGPSupdate = tNow; // remember time of this update (local time)
    Serial.println("time set from GPS");
      
  }

} 

//#############################################################################
// GPSinit
//#############################################################################
void GPSinit(uint8_t gps)
{
    tGPSupdate = 0;      // reset GPS last update time
    GPSupdating = false; // GPS not updating yet
}

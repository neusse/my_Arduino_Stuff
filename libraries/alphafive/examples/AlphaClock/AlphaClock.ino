/*

AlphaClock.ino 

Firmware for the Alpha Clock Five by William B Phelps
Version 2.1.13 - 04 February 2013
GPS and DST support Copyright 2013 (c) William B. Phelps - all commercial rights reserved

FIXES:
- single point time/date fetch to keep values in sync
- fix bug if time changes while setting date
- fix bug when Month (& Day) wrap
- allow turning alarm off when snoozing
- entering menu mode cancels LED test mode
- don't repeat on Menu & Test buttons
- fix bug in menu display for 1 word items
- set RTC from GPS when updating clock time
- fix TZ_Minutes adjustment (oops!)
- TZ Hour can go to +14, -12

CHANGES:
- rewrite DisplayWord/DisplayWordSequence
- 2 segment seconds spinner
- blink alarm indicator if alarm sounding or snoozed

- Automatic DST (currently for US only)
- set time & date from GPS
- turn "VCR mode" off when time set from GPS

OPTIONAL (disabled, see comments on how to enable):
- Hold S1+S2 to enter menu, S3+S4 to enter test mode
- Reverse +/- buttons

TODO:
- add DST configuration to menu

 ------------------------------------------------------------

 AlphaClock.ino 
 
 -- Alpha Clock Five Firmware, version 2.1 --
 
 Version 2.1.0 - January 31, 2013
 Copyright (c) 2013 Windell H. Oskay.  All right reserved.
 http://www.evilmadscientist.com/
 
 ------------------------------------------------------------
 
 Designed for Alpha Clock Five, a five letter word clock designed by
 Evil Mad Scientist Laboratories http://www.evilmadscientist.com
 
 Target: ATmega644A, clock at 16 MHz.
 
 Designed to work with Arduino 1.0.3; untested with other versions.
 
 For additional requrements, please see:
 http://wiki.evilmadscience.com/Alpha_Clock_Firmware_v2
 
 
 Thanks to Trammell Hudson for inspiration and helpful discussion.
 https://bitbucket.org/hudson/alphaclock
 
 
 Thanks to William Phelps - wm (at) usa.net, for several important 
 bug fixes.    https://github.com/wbphelps/AlphaClock
 
 ------------------------------------------------------------
 
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this library.  If not, see <http://www.gnu.org/licenses/>.
  
 Note that the two word lists included with this distribution are NOT licensed under the GPL.
 - The list in fiveletterwords.h is derived from SCOWL, http://wordlist.sourceforge.net 
 Please see README-SCOWL.txt for copyright restrictions on the use and redistribution of this word list.
 - The alternative list in fiveletterwordspd.h is in the PUBLIC DOMAIN, 
 and cannot be restricted by the GPL or other copyright licenses.    
 	  
 */
 
// Defining these enables the corresponding feature
// Note - GPS support has not been tested without AUTODST enabled
#define FEATURE_AUTODST
#define FEATURE_WmGPS

// NOTE: uncomment either of the following two "define" statements to enable the optional feature
// Use S1+S2 for Menu, S3+S4 for test mode
//#define MENU_S1S2
// Flip the +/- buttons
//#define REVERSE_PMBTNS

#include <Time.h>       // The Arduino Time library, http://www.arduino.cc/playground/Code/Time
#include <Wire.h>       // For optional RTC module
#include <DS1307RTC.h>  // For optional RTC module. (This library included with the Arduino Time library)
#include <EEPROM.h>     // For saving settings 

#include "alphafive.h"      // Alpha Clock Five library

#ifdef FEATURE_AUTODST
#include "adst.h"
#endif
void  EndVCRmode(void);
#ifdef FEATURE_WmGPS
#include "gps.h"  // wbp GPS support
#endif

// Comment out exactly one of the following two lines
#include "fiveletterwords.h"   // Standard word list --
//#include "fiveletterwordspd.h" // Public domain alternative

// "Factory" default configuration can be configured here:
#define a5brightLevelDefault 9 
#define a5HourMode24Default 0
#define a5AlarmEnabledDefault 0
#define a5AlarmHrDefault 7  
#define a5AlarmMinDefault 30
#define a5NightLightTypeDefault 0
#define a5AlarmToneDefault 2
#define a5NumberCharSetDefault 2
#define a5DisplayModeDefault 0
#ifdef FEATURE_AUTODST
#define a5DSTModeDefault 0  // default no DST
#endif
#ifdef FEATURE_WmGPS
#define a5GPSModeDefault 0  // default no GPS
#define a5TZHourDefault -8  // default Pacific Time zone
#define a5TZMinutesDefault 0  // default Pacific Time zone
#endif

// Clock mode variables

byte HourMode24;
byte AlarmEnabled; // If the "ALARM" function is currently turned on or off. 
byte AlarmTimeHr;
byte AlarmTimeMin;
int8_t AlarmTone;

int8_t NightLightType;  
byte NightLightSign;
unsigned int NightLightStep; 

// Configuration menu:
byte menuItem;   //Current position within options menu
int8_t optionValue; 

#define AMPM24HRMenuItem 0
#define NightLightMenuItem 1
#define AlarmToneMenuItem 2
#define SoundTestMenuItem 3
#define numberCharSetMenuItem 4
#define DisplayStyleMenuItem 5
#define SetYearMenuItem 6
#define SetMonthMenuItem 7
#define SetDayMenuItem 8
#define SetSecondsMenuItem 9 
#define AltModeMenuItem 10 
#define MenuItemsMax 10

#ifdef FEATURE_AUTODST
#define DSTMenuItem 11
#define MenuItemsMax 11
#endif
#ifdef FEATURE_WmGPS
#define GPSMenuItem 12
#define TZHoursMenuItem 13
#define TZMinutesMenuItem 14
#define MenuItemsMax 14
#endif

// Clock display mode:
int8_t DisplayMode;   
int8_t DisplayModeLocalLast;
byte DisplayModePhase;
byte DisplayModePhaseCount;
byte VCRmode;
byte modeShowMenu;
byte modeShowDateViaButtons;
byte modeLEDTest;
byte UpdateEE; 
int8_t numberCharSet;

// Other global variables:
time_t tNow;  // set each pass thru Loop
byte UseRTC;
unsigned long NextClockUpdate, NextAlarmCheck;
unsigned long milliTemp;
unsigned int FLWoffset; // Counter variable for FLW (Five Letter Word) display mode

// Text Display Variables: 
unsigned long DisplayWordEndTime;
char wordCache[5];
char dpCache[5];

//byte wordSequence;
byte wordSequenceStep;
byte modeShowText;

// Word Display variables
char Word1[5];
char Word2[5];
char Word3[5];
char Word4[5];
unsigned int wordDuration;
byte wordCount;

byte RedrawNow, RedrawNow_NoFade;

// Button Management:
#define ButtonCheckInterval 20    // Time delay between responding to button state, ms
#define MenuHoldDownTime 2000     // How long to hold buttons to acces the Menu
#define TestHoldDownTime 4000     // How long to hold buttons to access LED test mode (wbp)

byte buttonStateLast;
byte buttonMonitor;
unsigned long Btn1_AlrmSet_StartTime, Btn2_TimeSet_StartTime, Btn3_Plus_StartTime, Btn4_Minus_StartTime;
unsigned long NextButtonCheck, LastButtonPress;

byte UpdateAlarmState, UpdateBrightness;
byte AlarmTimeChanged, TimeChanged;
byte holdDebounce;

// Brightness steps for manual brightness adjustment
byte Brightness;
#define BrightnessMax 12
byte MBlevel[] = { 0, 1, 5,10,15,19,15,19, 2, 5,10,15,19}; 
byte MBmode[]  = { 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2};

// For fade and update management: 
byte SecLast; 
byte MinNowOnesLast;
byte MinAlarmOnesLast;

//Alarm variables
byte AlarmTimeSnoozeMin;
byte AlarmTimeSnoozeHr;
byte alarmSnoozed;
byte alarmPrimed;
byte alarmNow;

byte modeShowAlarmTime;
byte SoundSequence; 

void incrementAlarm(void)
{  // Advance alarm time by one minute 

  AlarmTimeMin += 1;
  if (AlarmTimeMin > 59)
  {
    AlarmTimeMin = 0;
    AlarmTimeHr += 1; 
    if (AlarmTimeHr > 23)
      AlarmTimeHr = 0; 
  } 
  UpdateEE = 1;
}

void decrementAlarm(void)
{  // Retard alarm time by one minute 

  if (AlarmTimeMin > 0)
    AlarmTimeMin--;
  else
  {   
    AlarmTimeMin = 59;
    if (AlarmTimeHr > 0)
      AlarmTimeHr--;
    else
      AlarmTimeHr = 23;
  }
  UpdateEE = 1;
}

void TurnOffAlarm(void)
{ // This cancels the alarm when it is going off (or alarmSnoozed).
  // It does leave the alarm enabled for next time, however.
  if (alarmNow || alarmSnoozed){
    alarmSnoozed = 0;
    alarmNow = 0;
    a5noTone();

    if (modeShowMenu == 0) 
			DisplayWords("ALARM", "STOP ", 800); // Display: "ALARM OFF", EXCEPT if we are in the menus.
  }
}  

void checkButtons(void )
{ 
  buttonMonitor |= a5GetButtons(); 

  if (milliTemp  >=  NextButtonCheck)  // Typically, go through this every 20 ms.
  {
    NextButtonCheck = milliTemp + ButtonCheckInterval;
    /*
     #define a5alarmSetBtn  1				// Snooze/Set alarm button
     #define a5timeSetBtn   2				// Set time button
     #define a5plusBtn      4				// + button
     #define a5minusBtn     8				// - button
     */

    if (buttonMonitor){  // If any buttons have been down in last ButtonCheckInterval

      if (VCRmode)
        EndVCRmode();  // Turn off VCR-blink mode, if it was still on.


      // Check to see if any of the buttons has JUST been depressed: 

      if (( buttonMonitor & a5_alarmSetBtn) && ((buttonStateLast & a5_alarmSetBtn) == 0))
      {  // If Alarm Set button was just depressed

        Btn1_AlrmSet_StartTime = milliTemp;  

        if (alarmNow){  // If alarm is going off, this button is the SNOOZE button.

          if (modeShowMenu)
          {
            TurnOffAlarm();
          }
          else{
            alarmNow = 0;
            a5noTone();
            alarmSnoozed = 1; 

            a5editFontChar ('a', 54, 1, 37);    // Define special character
            DisplayWord("SNaZE", 1500);  

            AlarmTimeSnoozeMin = minute(tNow) + 9;
            AlarmTimeSnoozeHr = hour(tNow);

            if  ( AlarmTimeSnoozeMin > 59){
              AlarmTimeSnoozeMin -= 60;
              AlarmTimeSnoozeHr += 1;
            }
            if (AlarmTimeSnoozeHr > 23)
              AlarmTimeSnoozeHr -= 24; 
          }

        } 
      }

      if (( buttonMonitor & a5_timeSetBtn) && ((buttonStateLast & a5_timeSetBtn) == 0)){
        // Button S2 just depressed.
        Btn2_TimeSet_StartTime = milliTemp; 
        TimeChanged = 0;
      }
      if (( buttonMonitor & a5_plusBtn) && ((buttonStateLast & a5_plusBtn) == 0))
        Btn3_Plus_StartTime = milliTemp; 
      if (( buttonMonitor & a5_minusBtn) && ((buttonStateLast & a5_minusBtn) == 0)) 
        Btn4_Minus_StartTime = milliTemp;  
    }
    else if ((buttonStateLast == 0 ) && ( buttonMonitor == 0))
    {
      // Reset some variables if all buttons are up, and have not just been released:
      TimeChanged = 0;
      AlarmTimeChanged = 0;
      holdDebounce = 1;  
    }

    if (modeShowMenu || buttonMonitor)
      LastButtonPress = milliTemp;  //Reset EEPROM Save Timer if menu shown, or if a button pressed.

    /////////////////////////////  ENTERING & LEAVING CONFIG MENU  /////////////////////////////  
#ifdef MENU_S1S2
    // Check to see if both S1 and S2 are both currently pressed or held down:
    if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn) && holdDebounce)
    {
      if( (milliTemp >= (Btn1_AlrmSet_StartTime + MenuHoldDownTime )) && (milliTemp >= (Btn2_TimeSet_StartTime + MenuHoldDownTime )))
      {  
        Btn1_AlrmSet_StartTime = milliTemp;     // Reset hold-down timer
        Btn2_TimeSet_StartTime = milliTemp;    // Reset hold-down timer
        holdDebounce = 0;
        TurnOffAlarm();
        if (modeShowMenu) // If we are currently in the configuration menu, 
        { 
          modeShowDateViaButtons = 0;  // don't show date on exit
          modeShowMenu = 0;  //  Exit configuration menu     
          DisplayWord("     ", 500); 
        }
        else
        {
          modeLEDTest = 0;  //  Exit LED Test Mode if on
          modeShowDateViaButtons = 0;  // stop showing date
          modeShowMenu = 1;  // Enter configuration menu 
          menuItem = 0; 
          DisplayWord("     ", 500); 
        }
      }
      else
      {  
        if (modeShowDateViaButtons == 0)
        { // Display date
          modeShowDateViaButtons = 1;
          TimeChanged  = 1; // This overrides the usual alarm on/off function of the time set button. 
          RedrawNow = 1; 
        }
      } 
    }
#else
    // Check to see if both S3 and S4 are both currently pressed or held down:
    if (( buttonMonitor & a5_plusBtn) && ( buttonMonitor & a5_minusBtn) && holdDebounce)
    {
      if( (milliTemp >= (Btn3_Plus_StartTime + MenuHoldDownTime )) && (milliTemp >= (Btn4_Minus_StartTime + MenuHoldDownTime )))
      {  
        Btn3_Plus_StartTime = milliTemp;     // Reset hold-down timer
        Btn4_Minus_StartTime = milliTemp;    // Reset hold-down timer
        holdDebounce = 0;
        TurnOffAlarm();
        if (modeShowMenu) // If we are currently in the configuration menu, 
        { 
          modeShowMenu = 0;  //  Exit configuration menu     
          DisplayWord("     ", 500); 
        }
        else
        {
          modeLEDTest = 0;  //  Exit LED Test Mode if on
          modeShowDateViaButtons = 0;  // stop showing date
          modeShowMenu = 1;  // Enter configuration menu 
          menuItem = 0; 
          DisplayWord("     ", 500); 
        }
      }
    }
#endif

    if (modeShowMenu && holdDebounce){    // Button behavior, when in Config menu mode:

      // Check to see if AlarmSet button was JUST released::
      if ( ((buttonMonitor & a5_alarmSetBtn) == 0) && (buttonStateLast & a5_alarmSetBtn))
      {  
        if ( menuItem > 0)   //Decrement current position within options menu
          menuItem--;
        else
          menuItem = MenuItemsMax;  // Wrap-around at low-end of menu
        optionValue = 0;
        TurnOffAlarm();
        DisplayMenuOptionName();
      }

      // If TimeSet button was just released::
      if (( (buttonMonitor & a5_timeSetBtn) == 0) && (buttonStateLast & a5_timeSetBtn))
      {
        if ((alarmNow) || (alarmSnoozed)){  // Just Turn Off Alarm 
          TurnOffAlarm();
        } 
        else {  // If we are in a configuration menu
          menuItem++;          
          if ( menuItem > MenuItemsMax)   //Decrement current position within options menu
            menuItem = 0;  // Wrap-around at high-end of menu
          optionValue = 0;
          TurnOffAlarm();
          DisplayMenuOptionName();
        }
      }

      if (( (buttonMonitor & a5_plusBtn) == 0) && (buttonStateLast & a5_plusBtn))
      { // The "+" button has just been released.
        optionValue = 1; 
        UpdateEE = 1;
        RedrawNow = 1;
      }

      if (( (buttonMonitor & a5_minusBtn) == 0) && (buttonStateLast & a5_minusBtn)) 
      {  // The "-" Button has just been released.
        optionValue = -1; 
        UpdateEE = 1;
        RedrawNow = 1;
      }
    }
    else
    {   // Button behavior, when NOT in Config menu mode: 

      /////////////////////////////  Time-Of-Day Adjustments  /////////////////////////////  

      // Check to see if both time-set button and plus button are both currently depressed:
      if (( buttonMonitor & a5_TimeSetPlusBtns) == a5_TimeSetPlusBtns) 
        if (TimeChanged < 2)
        {        
          adjustTime(60); // Add one minute   
          RedrawNow = 1; 
          TimeChanged = 2;  // One-time press: detected
          if (UseRTC)  
            RTC.set(now()); 
        }    
        else if ( milliTemp >= (Btn3_Plus_StartTime + 400))
        {
          adjustTime(60); // Add one minute 
          RedrawNow_NoFade = 1; 
          if (UseRTC)  
            RTC.set(now());      
        }

      // Check to see if both time-set button and minus button are both currently depressed:
      if (( buttonMonitor & a5_TimeSetMinusBtns) == a5_TimeSetMinusBtns) 
        if (TimeChanged < 2)
        {        
          adjustTime(-60); // Subtract one minute 
          RedrawNow = 1; 
          TimeChanged = 2;
          if (UseRTC)  
            RTC.set(now()); 
        }      
        else if ( milliTemp > (Btn4_Minus_StartTime + 400 ))
        {
          adjustTime(-60); // Subtract one minute 
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;    
          if (UseRTC)  
            RTC.set(now());      
        }

      /////////////////////////////  Time-Of-Alarm Adjustments  /////////////////////////////  

      // Entering alarm mode:
      // If Alarm button has been down 40 ms, 
      //    (to avoid displaying alarm if Alarm+Time buttons are pressed at the same time)
      //    the Set Time button is not down,
      //    and no other high-priority modes are enabled...

      if (( buttonMonitor & a5_alarmSetBtn) && (modeShowAlarmTime == 0))
        if ((( buttonMonitor & a5_timeSetBtn) == 0) && (modeShowText == 0))
          if ( milliTemp >= (Btn1_AlrmSet_StartTime + 40 ))  // of those "ifs," Check hold-time LAST.
          {
            modeShowAlarmTime = 1;
            RedrawNow = 1; 
            AlarmTimeChanged = 0; 
          }

      // Check to see if both alarm-set button and plus button are both currently depressed:
      if (( buttonMonitor & a5_AlarmSetPlusBtns) == a5_AlarmSetPlusBtns)  
        if (TimeChanged < 2)
        {        
          incrementAlarm(); // Add one minute
          RedrawNow = 1; 
          TimeChanged = 2;  // One-time press: detected
          alarmSnoozed = 0;  //  Recalculating alarm time *turns snooze off.*
        }       
        else if ( milliTemp >= (Btn3_Plus_StartTime + 400))
        {
          incrementAlarm(); // Add one minute
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;         
        }      

      // Check to see if both alarm-set button and minus button are both currently depressed:
      if (( buttonMonitor & a5_AlarmSetMinusBtns) == a5_AlarmSetMinusBtns) 
        if (TimeChanged < 2)
        {        
          decrementAlarm(); // Subtract one minute
          RedrawNow = 1; 
          TimeChanged = 2; // One-time press: detected
          alarmSnoozed = 0;  //  Recalculating alarm time *turns snooze off.*
        }      
        else if ( milliTemp >  (Btn4_Minus_StartTime + 400))
        {
          decrementAlarm(); // Subtract one minute
          RedrawNow_NoFade = 1; 
          //          TimeChanged = 1;         
        }

      /////////////////////////////  ENTERING & LEAVING LED TEST MODE  /////////////////////////////  
#ifdef MENU_S1S2
      // Check to see if both S3 and S4 are both currently held down:
      if (( buttonMonitor & a5_plusBtn) && ( buttonMonitor & a5_minusBtn) && holdDebounce)
      {
        if( (milliTemp >= (Btn3_Plus_StartTime + TestHoldDownTime )) && (milliTemp >= (Btn4_Minus_StartTime + TestHoldDownTime )))
        {  
          Btn3_Plus_StartTime = milliTemp;     // Reset hold-down timer
          Btn4_Minus_StartTime = milliTemp;    // Reset hold-down timer
          holdDebounce = 0;
          if (modeLEDTest) // If we are currently in the LED Test mode,
          {
            modeLEDTest = 0;  //  Exit LED Test Mode    
            RedrawNow = 1; 
            DisplayWord("-END-", 1500);
          }
          else
          { 
            // Display version and enter LED Test Mode
            DisplayWords("V21WM", " LED ", "TEST ", 2000);
            wordDuration = 1000;  // shorter duration for the rest
            DisplayWordDP("_1___");
            modeLEDTest = 1;
            SoundSequence = 0;
          }
        }
      }
#else
      /////////////////////////////  ENTERING & LEAVING LED TEST MODE  /////////////////////////////  
      // Check to see if both S1 and S2 are both currently depressed or held down:
      if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn) && holdDebounce)
      {
        if( (milliTemp >= (Btn1_AlrmSet_StartTime + TestHoldDownTime )) && (milliTemp >= (Btn2_TimeSet_StartTime + TestHoldDownTime )))
        {
          Btn1_AlrmSet_StartTime = milliTemp;  // Reset hold-down timer
          Btn2_TimeSet_StartTime = milliTemp;   // Reset hold-down timer
          holdDebounce = 0;
          if (modeLEDTest) // If we are currently in the LED Test mode,
          {
            modeShowDateViaButtons = 0;  // reset Date display
            modeLEDTest = 0;  //  Exit LED Test Mode    
            RedrawNow = 1; 
            DisplayWord("-END-", 1500);
          }
          else
          { 
            // Display version and enter LED Test Mode
            DisplayWords("V21WM", " LED ", "TEST ", 2000);
            wordDuration = 1000;  // shorter duration for the rest
            DisplayWordDP("_1___");
            modeLEDTest = 1;
            SoundSequence = 0;
          }
        }
        else  // both buttons down but not held
        {  
          if (modeShowDateViaButtons == 0)
          { // Display date
            modeShowDateViaButtons = 1;
            TimeChanged  = 1; // This overrides the usual alarm on/off function of the time set button. 
            RedrawNow = 1; 
          }
        }
      }
#endif

      // Check to see if AlarmSet button was JUST released::
      if ( ((buttonMonitor & a5_alarmSetBtn) == 0) && (buttonStateLast & a5_alarmSetBtn))
      {  
        if (modeShowAlarmTime && holdDebounce){  
          modeShowAlarmTime = 0; 
          RedrawNow = 1;
        } 
        if (modeShowDateViaButtons == 1)
        {
          modeShowDateViaButtons = 0;
          RedrawNow = 1; 
        } 
      }


      // If TimeSet button was just released::
      if (( (buttonMonitor & a5_timeSetBtn) == 0) && (buttonStateLast & a5_timeSetBtn))
      {
        if (holdDebounce)
        { 
          if ((alarmNow) || (alarmSnoozed)){  // Just Turn Off Alarm 
            TurnOffAlarm();
          } 
          else if (TimeChanged == 0){  // If the time has just been adjusted, DO NOT change alarm status.
            RedrawNow = 1;
            UpdateEE = 1;
            if (AlarmEnabled)
              AlarmEnabled = 0; 
            else
            {
              AlarmEnabled = 1; 
            } 
          }
          else
          {
            if (UseRTC)  
              RTC.set(now()); 
          }
        }

      }


      if (( (buttonMonitor & a5_plusBtn) == 0) && (buttonStateLast & a5_plusBtn))
      { // The "+" button has just been released.
        if (holdDebounce)
        { 
          if (TimeChanged > 0)
            TimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 
          if (AlarmTimeChanged > 0)
            AlarmTimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 

          // IF no other buttons are down, increase brightness:
          if (((buttonMonitor & a5_allButtonsButPlus) == 0) && (AlarmTimeChanged + TimeChanged == 0))
            if (Brightness < BrightnessMax)
            {
              Brightness++; 
              UpdateBrightness = 1;
              UpdateEE = 1;
            } 
        }
      }

      if (( (buttonMonitor & a5_minusBtn) == 0) && (buttonStateLast & a5_minusBtn)) 
      {  // The "-" Button has just been released.
        if (holdDebounce){
          if (TimeChanged > 0)
            TimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 
          if (AlarmTimeChanged > 0)
            AlarmTimeChanged = 1;  // Acknowledge that the button has been released, for purposes of time editing. 

          // IF no other buttons are down, and times have not been adjusted, decrease brightness:
          if(((buttonMonitor & a5_allButtonsButMinus) == 0) && (AlarmTimeChanged + TimeChanged == 0))
            if (Brightness > 0)
            {
              Brightness--; 
              UpdateBrightness = 1; 
              UpdateEE = 1;
            }  
        }
      }
    } // End not-in-config-menu statements

    buttonStateLast = buttonMonitor;
    buttonMonitor = 0;
  }
}


void DisplayMenuOptionName(void){
  // Display title of menu name after switching to new menu utem.
  // Turn off word sequence if running
  wordCount = 0;
  wordSequenceStep = 0;
  switch (menuItem) {
  case NightLightMenuItem:
    DisplayWords("NIGHT", "LIGHT", 600);  // Night Light
    break;
  case AlarmToneMenuItem:
    DisplayWords("ALARM", "TONE ", 700);  // Alarm Tone
    break; 
  case SoundTestMenuItem:
    DisplayWords("SOUND", "TEST ", 600); // Sound-test menu item, 3.  Display "TEST" "SOUND" "USE+-"
    break;
  case numberCharSetMenuItem:
    DisplayWords("FONT ", "STYLE", 600); // Font Style
    break;
  case DisplayStyleMenuItem:
    DisplayWords("CLOCK", "STYLE", 600); // Clock Style
    break;
  case SetYearMenuItem:
    DisplayWord("YEAR ", 800); 
    DisplayWordDP("___12");
    break;
  case SetMonthMenuItem:
    DisplayWord("MONTH", 800);   
    break;      
  case SetDayMenuItem:
    DisplayWord("DAY  ", 800);  
    DisplayWordDP("__12_");
    break;       
  case SetSecondsMenuItem:
    DisplayWord("SECS ", 800);  
    DisplayWordDP("___12");
    break;   
  case AltModeMenuItem:  
    DisplayWords("TIME ", "AND..", 900);
    //    DisplayWord("ALTW/", 2000);  
    //   DisplayWordDP("__11_");
    break;
#ifdef FEATURE_AUTODST
  case DSTMenuItem:
    DisplayWord("DST  ", 800);
    break;
#endif
#ifdef FEATURE_WmGPS
  case GPSMenuItem:
    DisplayWord("GPS  ", 800);
    break;
  case TZHoursMenuItem:
    DisplayWords("TIME ", "ZONE ", "HOUR ", 600);  // Time Zone Hour
    break;
  case TZMinutesMenuItem:
    DisplayWords("TIME ", "ZONE ", "MIN  ", 600);  // Time Zone Min
    break;
#endif
  default:  // do nothing!
    break;
  }
}


void ManageAlarm (void) {

  if ((SoundSequence == 0) && (modeShowMenu == 0))
    DisplayWord("ALARM", 400);  // Synchronize with sounds!  
  //RedrawNow_NoFade = 1;

  if ( (TIMSK1 & _BV(OCIE1A)) == 0)  { // If last tone has finished   

    if (AlarmTone == 0)   // X-Low Tone
    {
      if (SoundSequence < 8)
      {
        if (SoundSequence & 1) 
          a5tone( 50, 300);  
        else 
          a5tone(0, 300);  
        SoundSequence++;    
      }
      else
      { 
        a5tone(0, 1200);
        SoundSequence = 0;
      }
    }
    else if (AlarmTone == 1)   // Low Tone
    {
      if (SoundSequence < 8)
      {
        if (SoundSequence & 1) 
          a5tone( 100, 200);  
        else 
          a5tone(0, 200);  
        SoundSequence++;    
      }
      else
      { 
        a5tone(0, 1200);
        SoundSequence = 0;
      }
    }
    else  if (AlarmTone == 2) // Med Tone
    {
      if (SoundSequence < 6)
      {
        if (SoundSequence & 1) 
          a5tone( 1000, 200); 
        else 
          a5tone( 0, 200); 
        SoundSequence++;   
      }
      else
      { 
        a5tone( 0, 1400);
        SoundSequence = 0; 
      }
    }
    else  if (AlarmTone == 3) // High Tone
    {
      if (SoundSequence < 6) 
      {
        if (SoundSequence & 1) 
          a5tone( 2050, 300); 
        else 
          a5tone( 0, 200); 
        SoundSequence++;   
      }
      else
      { 
        a5tone(0, 1000);
        SoundSequence = 0;  
      }
    }
    else if (AlarmTone == 4)   // Siren Tone
    { 
      if (SoundSequence < 254)
      { 
        a5tone(20 + 4 * SoundSequence, 2); 
        SoundSequence++; 
      }
      else if (SoundSequence == 254)
      { 
        a5tone(20 + 4 * SoundSequence, 1500); 
        SoundSequence++;
      } 
      else {
        a5tone(0, 1000);
        SoundSequence = 0; 
      } 
    }
    else if (AlarmTone == 5)   // "Tink" Tone
    {
      if (SoundSequence == 0)
      {
        a5tone( 1000, 50);  // was 50 
        SoundSequence++;
      } 
      else  if (SoundSequence == 1)
      { 
        a5tone(0, 1900);
        SoundSequence++;
      }
      else
      { 
        a5tone(0, 50);
        SoundSequence = 0;  
      }
    } 
  }
}


void DisplayWords (char WordIn1[], unsigned int duration)
{
  DisplayWords(WordIn1, "", "", "", duration, 1);
}
void DisplayWords (char WordIn1[], char WordIn2[], unsigned int duration)
{
  DisplayWords(WordIn1, WordIn2, "", "", duration, 2);
}
void DisplayWords (char WordIn1[], char WordIn2[], char WordIn3[], unsigned int duration)
{
  DisplayWords(WordIn1, WordIn2, WordIn3, "", duration, 3);
}
void DisplayWords (char WordIn1[], char WordIn2[], char WordIn3[], char WordIn4[], unsigned int duration)
{
  DisplayWords(WordIn1, WordIn2, WordIn3, WordIn4, duration, 4);
}
void DisplayWords (char WordIn1[], char WordIn2[], char WordIn3[], char WordIn4[], unsigned int duration, byte count)
{
  wordSequenceStep = 0;
//  wordSequence = count;  // anything that isn't zero
  strncpy(Word1, WordIn1, 5);
  strncpy(Word2, WordIn2, 5);
  strncpy(Word3, WordIn3, 5);
  strncpy(Word4, WordIn4, 5);
  wordDuration = duration;
  wordCount = count;
  DisplayWordSequence();  // start the display 
}

void DisplayWordSequence ()
{
  wordSequenceStep++;
  DisplayWordDP("_____"); // Blank DPs unless stated otherwise.
  if (wordSequenceStep < wordCount*2+1)
  {
    if (wordSequenceStep == 1)
      DisplayWord(Word1, wordDuration);
    else if (wordSequenceStep == 3)
      DisplayWord(Word2, wordDuration);
    else if (wordSequenceStep == 5)
      DisplayWord(Word3, wordDuration);
    else if (wordSequenceStep == 7)
      DisplayWord(Word4, wordDuration);
    else
      DisplayWord("     ", 200);
  }
  else
  {
//    wordSequence = 0;  // done with sequence, turn it off
    wordCount = 0;  // all done
  }
}


void DisplayWord (char WordIn[], unsigned int duration)
{ // Usage: DisplayWord("ALARM", 500); 
  modeShowText = 1;  
  wordCache[0] = WordIn[0];
  wordCache[1] = WordIn[1];
  wordCache[2] = WordIn[2];
  wordCache[3] = WordIn[3];
  wordCache[4] = WordIn[4];
  DisplayWordEndTime = milliTemp + duration;
  RedrawNow = 1; 
}

void DisplayWordDP (char WordIn[])
{
  // Usage: DisplayWord("_123_"); 
  // Add or edit decimals for text displayed via DisplayWord().
  // Call in conjuction with DisplayWord, just before or after.

  dpCache[0] = WordIn[0];
  dpCache[1] = WordIn[1];
  dpCache[2] = WordIn[2];
  dpCache[3] = WordIn[3];
  dpCache[4] = WordIn[4];
}

void  EndVCRmode(){ 
  if (VCRmode){
    a5_brightLevel = MBlevel[Brightness];  
    RedrawNow_NoFade = 1;
    VCRmode = 0;

    randomSeed(now());  // Either a button press or RTC time 
  }
}



void setup() {     

  a5Init();  // Required hardware init for Alpha Clock Five library functions

//  a5tone(220, 100);
//  delay(200);
//  a5tone(2220, 100);
//  delay(200);
//  a5tone(220, 100);

  VCRmode = 1;

  Serial.println("\nHello, World.");
  Serial.println("Alpha Clock Five here, reporting for duty!");   

#ifdef FEATURE_WmGPS
  Serial1.end();  // close serial1 opened by a5Init
  Serial1.begin(9600);  // re-open at 9600 bps for GPS
  GPSinit(96);  // assume 9600 bps (Adafruit Ultimate GPS)
  Serial.println("GPS on Serial1 enabled");
#endif

  EEReadSettings(); // Read settings stored in EEPROM

  if (Brightness == 0)
    Brightness = 1;       // If display is fully dark at reset, turn it up to minimum brightness.

  UseRTC = a5CheckForRTC();
  if (UseRTC)   
  {
    setSyncProvider(RTC.get);   // Function to get the time from the RTC (e.g., Chronodot)
    if(timeStatus()!= timeSet) { 
      Serial.println("RTC detected, *but* I can't seem to sync to it. ;("); 
      UseRTC = 0;
    }
    else {
      Serial.println("System time: Set by RTC.  Rock on!");  
      EndVCRmode();
    }
  }
  else
    Serial.println("RTC not detected. I don't know what time it is.  :(");  

  if ( UseRTC == 0)
  {
    Serial.println("Setting the date to 2013. I didn't exist in 1970.");   
    setTime(0,0,0,1, 1, 2013);
  }

#ifdef FEATURE_AUTODST
  if (DST_mode)
    DSTinit(now(), DST_Rules);  // re-compute DST start, end
#endif

  SerialPrintTime(); 
  NextClockUpdate = millis() + 1;

  buttonMonitor = 0;  
  holdDebounce = 0;

  modeShowAlarmTime = 0;
  modeShowDateViaButtons = 0; // for button-press date display
  modeShowMenu = 0;
  modeShowText = 0;
  modeLEDTest = 0;

  // Alarm Setup:
  alarmSnoozed = 0;
  alarmPrimed = 0;
  alarmNow = 0; 
  SoundSequence = 0; 

  NextButtonCheck = NextClockUpdate;
  NextAlarmCheck =  NextClockUpdate;

  UpdateEE = 0;
  LastButtonPress = NextClockUpdate;

  wordCount = 0;
  wordSequenceStep = 0;

  RedrawNow = 1;
  RedrawNow_NoFade = 0;
  UpdateBrightness = 0;

  DisplayWords("ALPHA", "CLOCK", "  5  ", "     ", 750);  // Display: Hello world

  buttonMonitor = a5GetButtons(); 
  if (( buttonMonitor & a5_alarmSetBtn) && ( buttonMonitor & a5_timeSetBtn))
  {
    // If Alarm button and Time button (LED Test buttons) held down at turn on, reset to defaults.
    Brightness = a5brightLevelDefault; 
    HourMode24 = a5HourMode24Default; 
    AlarmEnabled = a5AlarmEnabledDefault; 
    AlarmTimeHr = a5AlarmHrDefault; 
    AlarmTimeMin = a5AlarmMinDefault; 
    AlarmTone = a5AlarmToneDefault; 
    NightLightType = a5NightLightTypeDefault;   
    numberCharSet = a5NumberCharSetDefault; 
    DisplayMode = a5DisplayModeDefault;       
#ifdef FEATURE_AUTODST
    DST_mode = a5DSTModeDefault;
#endif
#ifdef FEATURE_WmGPS
    GPS_mode = a5GPSModeDefault;
#endif
    wordSequenceStep = 0;
    DisplayWord("*****", 1000); 
  }


  a5_brightLevel = MBlevel[Brightness];
  a5_brightMode = MBmode[Brightness]; 

  a5loadAltNumbers(numberCharSet);

  FLWoffset = 0;

  NightLightSign = 1;
  NightLightStep = 0; 
  updateNightLight();

  DisplayModePhase = 0;
  DisplayModePhaseCount = 0;

}

void loop() {

  tNow = now();  // fetch time & date (wbp)
  milliTemp = millis();
  checkButtons();

  if (UpdateBrightness)
  {
    UpdateBrightness = 0;  // Reset the flag that triggered this clause.

    if (a5_brightMode == MBmode[Brightness])
    {
      a5_brightLevel = MBlevel[Brightness];          
      UpdateDisplay (1); // Force update of display data, with new brightness level 
    }
    else
    {
      a5_brightLevel = 0;
      UpdateDisplay (1); // Force update of display data, with temporary brightness level
      a5loadVidBuf_fromOSB();

      a5_brightLevel = MBlevel[Brightness];          
      UpdateDisplay (1); // Force update of display data, with new brightness level  
      a5_brightMode = MBmode[Brightness];  
    } 
  }

  if (VCRmode) 
  {
    if (modeShowText == 0){
      byte temp = second(tNow) & 1; 
      if((temp) && (VCRmode == 1))
      {
        a5_brightLevel = 0;  
        RedrawNow_NoFade = 1;
        VCRmode = 2;
      }
      if((temp == 0) && (VCRmode == 2))
      {
        a5_brightLevel = MBlevel[Brightness];  
        RedrawNow_NoFade = 1;
        VCRmode = 1;
      }
    }
  }


  if (RedrawNow || RedrawNow_NoFade)
  { 
    NextClockUpdate = milliTemp + 10; // Reset auto-redraw timer.

    UpdateDisplay (1);   // Force redraw
    if (RedrawNow_NoFade)   // Explicitly do not fade.  Takes priority over redraw with fade.
      a5_FadeStage = -1;
    a5LoadNextFadeStage(); 
    a5loadVidBuf_fromOSB(); 

    RedrawNow = 0;
    RedrawNow_NoFade = 0;
  }  
  else if (milliTemp >= NextClockUpdate)  // Update at most 100 times per second
  {  
    NextClockUpdate = milliTemp + 10; // Reset auto-redraw timer.
    UpdateDisplay (0); // Argument 0: Only update if display data has changed.
    a5LoadNextFadeStage();
    a5loadVidBuf_fromOSB(); 

    if (NightLightType >= 4)  // Only in pulse mode do we need to regularly update
      updateNightLight();

    if (UpdateEE)   // Don't need to check this more than 100 times/second.
      EESaveSettings();
  }

  // Check for alarm:
  if  (milliTemp >= NextAlarmCheck)
  {
    NextAlarmCheck = milliTemp +  500;  // Check again in 1/2 second. 

    if (AlarmEnabled)  {
      byte hourTemp = hour(tNow);
      byte minTemp = minute(tNow);

      if ((AlarmTimeHr == hourTemp ) && (AlarmTimeMin == minTemp ))
      {
        if (alarmPrimed){ 
          alarmPrimed = 0;
          alarmNow = 1;
          alarmSnoozed = 0; 
          SoundSequence = 0; 
        }
      }
      else{
        alarmPrimed = 1;  
        // Prevent alarm from going off twice in the same minute, after being turned off and back on.
      }

      if (alarmSnoozed)
        if  ((AlarmTimeSnoozeHr == hourTemp ) && (AlarmTimeSnoozeMin == minTemp ))
        {
          alarmNow = 1;
          alarmSnoozed = 0; 
          SoundSequence = 0; 
        }
    }
  }

  if (alarmNow)
    ManageAlarm();

  if(Serial.available() ) 
  { 
    processSerialMessage();
  } 

#ifdef FEATURE_WmGPS
  if (GPS_mode)
  {
    if (Serial1.available())  //wbp
      getGPSdata();  //wbp
    if (GPSupdating)
      EndVCRmode();  // stop flashing the time 
  }
#endif

}



#define a5_COMM_MSG_LEN  13   // time sync to PC is HEADER followed by unix time_t as ten ascii digits  (Was 11)
#define a5_COMM_HEADER  255   // Header tag for serial sync messages

void SerialSendDataDaisyChain (char DataIn[])
{ 
  char outputBuffer[13];
  char *toPtr = &outputBuffer[0];
  char *fromPtr = &DataIn[0]; 

  *toPtr++ = 255;
  *toPtr++ = *fromPtr++;  
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;

  *toPtr++ = *fromPtr++;  
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;

  *toPtr++ = *fromPtr++;
  *toPtr++ = *fromPtr++;
  *toPtr = *fromPtr; 

  Serial1.write(outputBuffer);
}


void processSerialMessage() {

  char c,c2;
  byte i, temp, temp2;
  int8_t paramNo, valueNo;
  char OutputCache[13]; 

  // if time sync available from serial port, update time and return true
  while(Serial.available() >=  a5_COMM_MSG_LEN ){  // time message consists of a header and ten ascii digits

    if( Serial.read() == a5_COMM_HEADER) { 

      c = Serial.read() ; 
      c2 = Serial.read();

      if( c == 'S' )
      {
        if (c2 == 'T')
        {  // COMMAND: ST, SET TIME     
          time_t pctime = 0;
          for( i=0; i < 10; i++){   
            c = Serial.read();          
            if( c >= '0' && c <= '9'){   
              pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
            }
          }   
          setTime(pctime);   // Sync Arduino clock to the time received on the serial port
          DisplayWord("SYNCD", 900);
          DisplayWordDP("____2"); 
          Serial.println("PC Time Sync Signal Received.");
          SerialPrintTime(); 
          if (UseRTC)  
            RTC.set(now());
          EndVCRmode(); 
        } 
      }
      else if( c == 'B' )
      {
        if ((c2 == '0') || (c2 == 0)) // B0, with either ASCII or Binary 0.
        { // COMMAND: B0, Set Parameters 

          c = Serial.read();   // B0 command: Which setting to adjust
          c2 = Serial.read();  // Read first char of additional data

          if (c == '2')
          {
            // edit font character  
            // c2 : Idicates which ASCII character location to edit 
            // Read in 8 more ASCII chars:
            // [___][_][___] <- "A", "B", "C" values, ASCII text

            i = 100 * (Serial.read() - '0');
            i += 10 * (Serial.read() - '0');
            i += (Serial.read() - '0');

            temp = (Serial.read() - '0');          

            temp2 = 100 * (Serial.read() - '0');
            temp2 += 10 * (Serial.read() - '0');
            temp2 += (Serial.read() - '0');

            a5editFontChar(c2, i, temp, temp2); 

            Serial.read();  // Empty input buffer, char 10 of 10
          }
          else{         
            if (c == '0')
            {
              // Set brightness
              c = Serial.read();  // Read input buffer, char 3 of 10

              Brightness = (10 * (c2 - '0') + (c - '0'));
              UpdateBrightness = 1; 
            }
            if (c == '1')
            {// Load altnernate number set
              a5loadAltNumbers(c2 - '0'); 
              Serial.read();  // Empty input buffer, char 3 of 10
            }

            for( i=3; i < 10; i++){   
              Serial.read();  // Empty input buffer
            }   
          }

          RedrawNow = 1;  
          EndVCRmode();
        }
        else { // Daisy chaining: With Bx, where x is less than 48 or x is less than 10:
          if (c2 <= '9')  
          { // if we're here, c2 is <= '9', c2 != 0, and c2 != '0'.   

            OutputCache[0] = 'B';
            OutputCache[1] = c2 - 1;

            for( i=2; i < 12; i++){   
              OutputCache[i] = Serial.read();  
            }   
            SerialSendDataDaisyChain (OutputCache);            
          }

        }
      }
      else if( c == 'A' )
      {
        if ((c2 == '0') || (c2 == 0)) // A0, with either ASCII or Binary 0.
        { // COMMAND: A0, DISPLAY ASCII DATA  
          // ASCII display mode, first 5 chars will be displayed, second 5: decimals

          for( i=0; i < 10; i++){   
            c = Serial.read();     
            if (i < 5) 
              wordCache[i] = c; 
            else 
              dpCache[i - 5] = c;  
          }             
          modeShowText = 3;   
          RedrawNow = 1; 
          EndVCRmode();
        }
        else { // Daisy chaining: With Ax, where x is less than 48 or x is less than 10:
          if (c2 <= '9') 
          { // if we're here, c2 is <= '9', c2 != 0, and c2 != '0'.   

            OutputCache[0] = 'A';
            OutputCache[1] = c2 - 1;

            for( i=2; i < 12; i++){   
              OutputCache[i] = Serial.read();  
            }   
            SerialSendDataDaisyChain (OutputCache);            
          }
        }
      }

      else if( c == 'M' )  // Mode setting commands
      {// Eventually, it would be nice to have all settings and functions
        // accessible through the remote interface.

        if (c2 == 'T')   { // Command: 'MT' : Mode: Time
          modeShowAlarmTime = 0;
          modeShowMenu = 0;
          modeShowText = 0;
          modeLEDTest = 0;

          EndVCRmode();
        }
      }

    }
  }
}


void updateNightLight(void)
{
  if  (NightLightType == 4)  
  { // "Sleep" mode
    unsigned int temp = 0; 

    NightLightStep++;  

    if (NightLightStep <= 255) { 
      if (NightLightSign) 
        temp = NightLightStep; 
      else 
        temp = 255 - NightLightStep;    
    }
    else 
    { 
      if (NightLightSign) 
        temp = 255; 
      else 
        temp = 0; 

      if (NightLightStep > 280)
      {
        NightLightStep = 0; 
        if (NightLightSign)
          NightLightSign = 0;
        else 
          NightLightSign = 1; 
      }
    }

    temp = (temp * temp) >> 8;
    if (temp > 252) {
      temp = 252;
    }

    temp += 3;	  
    a5nightLight(temp);
  }
  else if (NightLightType == 0)  
    a5nightLight(0);   // OFF
  else if (NightLightType == 1) 
    a5nightLight(5);   // LOW
  else if (NightLightType == 2) 
    a5nightLight(50);   // MED  
  else if (NightLightType == 3) 
    a5nightLight(255);  // HIGH  
}


void UpdateDisplay (byte forceUpdate) { 

  byte temp, remainder;

  if (modeShowText)  //Text Display
  { 
    if ((milliTemp >= DisplayWordEndTime) && (modeShowText == 1))
    {
      modeShowText = 0;
      if (wordCount)  // if displaying a sequence of words,
        DisplayWordSequence();  // do the next word in the sequence
      // If the word sequence is finished, return to clock display:
      if (wordCount == 0) 
        RedrawNow = 1; 
    } 
    else
    {
      if(forceUpdate)
      { 
        a5clearOSB();    
        a5loadOSB_Ascii(wordCache,a5_brightLevel);
        a5loadOSB_DP(dpCache,a5_brightLevel);
        a5BeginFadeToOSB();   
      }
    }
  }
  else  if (modeLEDTest)  //LED Test Mode
  { 
    if (milliTemp > DisplayWordEndTime)
    {  
      forceUpdate = 1;
      SoundSequence++; 
      DisplayWordEndTime = milliTemp + 350;  // Advance every 350 ms.
    } 

    if(forceUpdate)
    { 
      // Borrow SoundSequence as a dummy variable:
      if (SoundSequence > 91)
        SoundSequence = 1;

      remainder = SoundSequence - 1;
      temp = 4;

      while (remainder >= 18){ 
        remainder -= 18;   // remainder
        temp -= 1;   // (4 - modulo)
      }

      byte map[] = {
        7,0,1,10,11,3,2,12,13,14,15,16,5,17,8,9,4,6
      };  

      a5clearOSB();   
      a5_OSB[18 * temp + map[remainder]] = a5_brightLevel; 
      a5BeginFadeToOSB();  
      RedrawNow = 1;
    }  

  }

  else if (modeShowMenu)
  {

    DisplayWordDP("_____");
    byte ExtendTextDisplay = 0; 

    if (menuItem == AMPM24HRMenuItem)  // Hour mode: 12Hr / 24 Hr
    {   
      if (optionValue != 0)
      {
        if (HourMode24)
          HourMode24 = 0;
        else
          HourMode24 = 1;
        optionValue = 0;
      }

      if (HourMode24) 
        DisplayWord("24 HR", 500);
      else
        DisplayWord("AM/PM", 500);  

      ExtendTextDisplay = 1;
    }
    else if (menuItem == NightLightMenuItem)  // Night Light
    {  
      NightLightType += optionValue;

      if (NightLightType < 0)
        NightLightType = 4;
      if (NightLightType > 4)
        NightLightType = 0;

      if (optionValue != 0){
        if (NightLightType == 4) 
        {
          NightLightStep = 0;
          NightLightSign = 1;  
        }
        updateNightLight();
      }
      optionValue = 0;

      if (NightLightType == 0) 
        DisplayWord(" NONE", 500);
      else if (NightLightType == 1) 
        DisplayWord(" LOW ", 500);  
      else if (NightLightType == 2) 
        DisplayWord(" MED ", 500);  
      else if (NightLightType == 3) 
        DisplayWord(" HIGH", 500);  
      else  // (NightLightType == 4) 
      DisplayWord("SLEEP", 500);  
      ExtendTextDisplay = 1;
    }    
    else if (menuItem == AlarmToneMenuItem)  // Alarm Tone: 2
    {  
      AlarmTone += optionValue;
      optionValue = 0;
      if (AlarmTone < 0)
        AlarmTone = 5;
      if (AlarmTone > 5)
        AlarmTone = 0;

      if (AlarmTone == 0) 
        DisplayWord("X LOW", 500);
      else if (AlarmTone == 1) 
        DisplayWord(" LOW ", 500);  
      else if (AlarmTone == 2) 
        DisplayWord(" MED ", 500);  
      else if (AlarmTone == 3) 
        DisplayWord(" HIGH", 500);   
      else if (AlarmTone == 4) 
        DisplayWord("SIREN", 500);   
      else 
        DisplayWord(" TINK", 500);   
      ExtendTextDisplay = 1;
    }   
    else if (menuItem == SoundTestMenuItem)  // Alarm Test: 3
    {   
      DisplayWord(" +/- ", 500);   
      if (optionValue != 0)
      { 
        if (alarmNow == 0)
          alarmNow = 1;
        else        
          TurnOffAlarm();
        optionValue = 0;
      }
      ExtendTextDisplay = 1;
    }   

    else if (menuItem == numberCharSetMenuItem)  
    {
      numberCharSet += optionValue;
      if (optionValue != 0){
        optionValue = 0;
        if (numberCharSet < 0)
          numberCharSet = 9;
        if (numberCharSet > 9)
          numberCharSet = 0;
        a5loadAltNumbers(numberCharSet);
      }

      DisplayWord("01237", 500); // Sample font display
      ExtendTextDisplay = 1;
    }   

    else if (menuItem == DisplayStyleMenuItem) 
    {  
      temp = (DisplayMode & 3U);

      if (optionValue != 0){ 
        if (optionValue == 1) 
          temp = (temp + 1) & 3U;  
        else if (temp == 0) 
          temp = 3; 
        else
          temp--;

        DisplayMode = (DisplayMode & 12U) | (temp);
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(DisplayMode & 3, forceUpdate); // Show clock time, in appropriate style
    }
    else if (menuItem == AltModeMenuItem)  // Alternate with seconds or date:
    {  
      // if (TimeDisplay & 4): Alternate date with time
      // if (TimeDisplay & 8): Alternate date with seconds
      // if (TimeDisplay & 16): Alternate date with words

      if (optionValue != 0)
      {
        temp = 1;
        if ( DisplayMode & 4)
          temp = 2;
        if ( DisplayMode & 8)
          temp = 3;
        if ( DisplayMode & 16)
          temp = 4;    

        temp += optionValue;

        if (temp == 0) 
          temp = 4; // Wrap around (low side)
        else if (temp == 5)
          temp = 0;  // wrap around (high side) 

        DisplayMode &= 3U;

        if (temp > 1)
          DisplayMode |= (1 << temp);
        // if temp is 0 or 1, display time only.

        DisplayModePhaseCount = 0;  
        optionValue = 0; 

      }

      if (DisplayMode & 4U)
        DisplayWord("DATE ", 500);
      else if (DisplayMode & 8U){
        DisplayWord("SECS ", 500);
        DisplayWordDP("___1_"); 
      }
      else if (DisplayMode & 16U){
        DisplayWord("WORDS", 500);
      }     
      else
        DisplayWord(" NONE", 500);

      ExtendTextDisplay = 1;

    } 
    else if (menuItem == SetYearMenuItem) 
    {  
      if (optionValue != 0){
        AdjDayMonthYear(0,0,optionValue); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1; 
      }    
      TimeDisplay(35, forceUpdate); // Show clock time, in appropriate style
    }

    else if (menuItem == SetMonthMenuItem) 
    {  
      if (optionValue != 0){  
        AdjDayMonthYear(0,optionValue,0); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(33, forceUpdate); // Show clock time, in appropriate style
    } 
    else if (menuItem == SetDayMenuItem) 
    {  
      if (optionValue != 0){ 
        AdjDayMonthYear(optionValue,0,0); // Day, Month, Year
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(33, forceUpdate); // Show clock time, in appropriate style
    }   

    else if (menuItem == SetSecondsMenuItem) 
    {  
      if (optionValue != 0){ 
        adjustTime(optionValue); // Adjust by +/- 1 second
        if (UseRTC)  
          RTC.set(now()); 
        optionValue = 0;
        forceUpdate = 1;
      }   
      TimeDisplay(32, forceUpdate); // Show clock time, seconds
    }    
 
 #ifdef FEATURE_AUTODST
   else if (menuItem == DSTMenuItem)
    {
      DST_mode += optionValue;

      if (DST_mode < 0)
        DST_mode = 2;
      if (DST_mode > 2)
        DST_mode = 0;

      optionValue = 0;

      if (DST_mode == 0) 
        DisplayWord(" OFF", 500);
      else if (DST_mode == 1) 
        DisplayWord(" ON  ", 500);  
      else // (DST_mode == 2) 
        DisplayWord(" AUTO", 500);  
      ExtendTextDisplay = 1;
    }
#endif
 
#ifdef FEATURE_WmGPS
   else if (menuItem == GPSMenuItem)
    {
      GPS_mode += optionValue;

      if (GPS_mode < 0)
        GPS_mode = 2;
      if (GPS_mode > 2)
        GPS_mode = 0;

      optionValue = 0;

      if (GPS_mode == 0) 
        DisplayWord(" OFF", 500);
      else if (GPS_mode == 1) 
        DisplayWord(" ON48", 500);  
      else // (GPS_mode == 2) 
        DisplayWord(" ON96", 500);  
      ExtendTextDisplay = 1;
    }

   else if (menuItem == TZHoursMenuItem)
    {
      TZ_hour += optionValue;
      if (TZ_hour < -12)
        TZ_hour = 14;  // there are 14 positive time zones, not 12 (Kiribati)
      if (TZ_hour > 14)
        TZ_hour = -12;
      if (optionValue != 0)
        tGPSupdateUT = 0;  // update time at next GPRMC
      optionValue = 0;
      TimeDisplay(41, forceUpdate); // Show TZ hour
    }
   else if (menuItem == TZMinutesMenuItem)
    {
      TZ_minutes += optionValue*15;
      if (TZ_minutes < 0)
        TZ_minutes = 45;
      if (TZ_minutes > 45)
        TZ_minutes = 0;
      if (optionValue != 0)
        tGPSupdateUT = 0;  // update time at next GPRMC
      optionValue = 0;
      TimeDisplay(42, forceUpdate); // Show TZ minutes
    }
#endif

    if(forceUpdate && ExtendTextDisplay)
    {  
      if (menuItem != DisplayStyleMenuItem) 
      {   
        a5clearOSB();    
        a5loadOSB_Ascii(wordCache,a5_brightLevel);
        a5loadOSB_DP(dpCache,a5_brightLevel);
        a5BeginFadeToOSB();   
      }
    }
  }
  else if (modeShowDateViaButtons) 
  { 
    TimeDisplay(33, forceUpdate); // Show date
  }
  else if (modeShowAlarmTime) 
  { 
    TimeDisplay(20, forceUpdate); // Show alarm time 
  }

  else
  {
    // Time Display Mode!  Possibly with aux. display.

    if ((DisplayMode > 3) && (DisplayMode < 32))
    {

     if (buttonMonitor) 
      {
        // Do not use alternate display modes when buttons are pressed.
        DisplayModePhase = 0;
        DisplayModePhaseCount = 0;
      }
      else
        if (DisplayModePhaseCount >= 7) // Alternate display every 7 seconds
        { 

          DisplayModePhaseCount = 0;

          DisplayModePhase++;
          if (DisplayModePhase > 1)
            DisplayModePhase = 0; 

          forceUpdate = 1;

          DisplayWord("     ", 400);  // Blank out between display phases 

          if (AlarmIndicate())  
            DisplayWordDP("2____");
          else
            DisplayWordDP("_____"); 

        }

      if (DisplayModePhase == 0)
      {
        TimeDisplay(DisplayMode & 3, forceUpdate);  // "Normal" time display
      }
      else
      { // Alternate display modes: "Time and ... "
        if (DisplayMode & 4)
          TimeDisplay(33, forceUpdate); // if Bit 4 is set: Show date
        else if (DisplayMode & 8)
          TimeDisplay(32, forceUpdate); // if Bit 8 is set: Show seconds 
        else if (DisplayMode & 16)
          TimeDisplay(36, forceUpdate); // if Bit 16 is set:  Show five letter words
      }
    }
    else 
      TimeDisplay(DisplayMode, forceUpdate);

  }
}

boolean AlarmIndicate (void)
{
  if ( AlarmEnabled && ( ((alarmNow == 0) && (alarmSnoozed == 0)) || (second(tNow)%2)) )  // flash alarm indicator if alarm going or snoozed
    return(true);
  else
    return(false);
}

void AdjDayMonthYear (int8_t AdjDay, int8_t AdjMonth, int8_t AdjYear)
{
  // From Time library: API starts months from 1, this array starts from 0
  const uint8_t monthDays[]={31,29,31,30,31,30,31,31,30,31,30,31};  // february can have 29 days

   time_t timeTemp = now();
      
  int yrTemp = year(timeTemp) + (int) AdjYear;  
  
  int moTemp = month(timeTemp) + AdjMonth;  // Avoid changing year, unless requested
  if (moTemp < 1)
      moTemp = 12;
   if (moTemp > 12)
      moTemp = 1;
      
  int dayTemp = day(timeTemp) + AdjDay;  // avoid changing month, unless requested
  
  if (dayTemp < 1)
     dayTemp = monthDays[moTemp - 1]; 
     
  if (dayTemp > monthDays[moTemp - 1])
    if (AdjDay > 0)
      {  // Roll over day-of-month to 1, if explicitly requesting increase in date.  
         dayTemp = 1;
      }
      else
      { // Otherwise, we should "truncate" the date to last day of month.
       dayTemp = monthDays[moTemp - 1];
      }
     
  setTime(hour(timeTemp),minute(timeTemp),second(timeTemp),
      dayTemp, moTemp, yrTemp);
  if (UseRTC)  
    RTC.set(now()); 
}


#if defined FEATURE_WmGPS || defined FEATURE_AUTODST
void setDSToffset(uint8_t mode) {
  int8_t adjOffset;
  uint8_t newOffset;
#ifdef FEATURE_AUTODST
  if (mode == 2) {  // Auto DST
    if (DST_updated) return;  // already done it once today
    newOffset = getDSToffset(now(), DST_Rules);  // get current DST offset based on DST Rules
  }
  else
#endif // FEATURE_AUTODST
    newOffset = mode;  // 0 or 1
  adjOffset = newOffset - DST_offset;  // offset delta
  if (adjOffset == 0)  return;  // nothing to do
  // play tones to indicate DST time adjustment, up or down
  if (adjOffset > 0)
  {
    a5tone(220, 100);
    delay(200);
    a5tone(2220, 100);
  }
  else
  {
    a5tone(2220, 100);
    delay(200);
    a5tone(220, 100);
  }
  time_t tNow = now();  // fetch current time & date
  tNow += adjOffset * SECS_PER_HOUR;  // add or subtract new DST offset
  setTime(tNow);  
  DST_updated = true;
  if (UseRTC)  
    RTC.set(now()); 
  DST_offset = newOffset;
//  eeprom_update_byte(&b_DST_offset, g_DST_offset);
  // save DST_updated in ee ???
}
#endif


void TimeDisplay (byte DisplayModeLocal, byte forceUpdateCopy)  {
  byte temp;
  byte temp1, temp2;  // wbp
  char units;
  char WordIn[] = { "     " };
  byte SecNowTens,  SecNowOnes;
  byte SecNow;

#ifdef FEATURE_AUTODST
// see if it's time to check for DST changes
  if (second(tNow)%10 == 0)  // check DST Offset every 10 seconds (60?)
    setDSToffset(DST_mode); 
  if ((hour(tNow) == 0) && (minute(tNow) == 0) && (second(tNow) == 0)) {  // MIDNIGHT!
    DST_updated = false;
    if (DST_mode == 2)  // if DST set to Auto
      DSTinit(tNow, DST_Rules);  // re-compute DST start, end
  }
#endif

  SecNow = second(tNow);

  if (SecLast != SecNow){
    forceUpdateCopy = 1;
    DisplayModePhaseCount++;
  }

  if ((DisplayModeLocal <= 4) || (DisplayModeLocal == 20))
  { // Normal time display OR Alarm time display
    // DisplayModeLocal 0: Standard-mode time-of-day display
    // DisplayModeLocal 1: Time-of-day w/ seconds spinner
    // DisplayModeLocal 2: Standard-mode time-of-day display + flashing separator
    // DisplayModeLocal 3: Time-of-day w/ seconds spinner + flashing separator

    // DisplayModeLocal 20: Standard-mode alarm-time display

    byte HrNowTens,  HrNowOnes, MinNowTens,  MinNowOnes;

    if (DisplayModeLocal == 20)
      temp = AlarmTimeHr;  
    else
      temp = hour(tNow); 

    if (HourMode24) 
      units = 'H';  
    else
    {  
        units = 'A'; 
       
      if (temp >= 12){
        units = 'P';   
      }       
      
      if (temp > 12){ 
        temp -= 12;   //
      }
       
      if (temp == 0)  // Represent 00:00 as 12:00
          temp += 12;
    }

    HrNowTens = U8DIVBY10(temp);    //i.e.,  HrNowTens = temp / 10;
    HrNowOnes = temp - 10 * HrNowTens;

    if (DisplayModeLocal == 20)
      temp = AlarmTimeMin;
    else
      temp = minute(tNow); 

    MinNowTens = U8DIVBY10(temp);      //i.e.,  MinNowTens = temp / 10;
    MinNowOnes = temp - 10 * MinNowTens;

    if (MinNowOnesLast != MinNowOnes)
      forceUpdateCopy = 1;

    SecNow = second(tNow);    

    if (SecLast != SecNow)
      forceUpdateCopy = 1;

    if (DisplayModeLocal & 1) // Seconds Spinner Mode
    {
      // Split seconds into octants: 0-6,7-14,15-22,23-29,30-36,37-44,45-52,53-59
      // segment to display:         15,   16,    5,   17,    8,    9,    4,   14 
//    byte segs[] = { 15, 16, 5, 17, 8, 9, 4, 14};  // wbp
//    temp = segs[(SecNow*2)/15];  // wbp
    byte segs1[] = { 15, 15, 16, 16, 5, 5, 17, 17, 8, 8, 9, 9, 4, 4, 14, 14};  // wbp
    byte segs2[] = { 0,  16,  0,  5, 0, 17, 0,  8, 0, 9, 0, 4, 0, 14, 0, 15};  // wbp
    temp1 = segs1[(SecNow*4)/15];  // wbp
    temp2 = segs2[(SecNow*4)/15];  // wbp
    }

    if ((HourMode24) || (HrNowTens > 0))
      WordIn[0] =  HrNowTens + a5_integerOffset;   // Blank leading zero unless in 24-hour mode.

    WordIn[1] =  HrNowOnes  + a5_integerOffset;
    WordIn[2] =  MinNowTens + a5_integerOffset;
    WordIn[3] =  MinNowOnes + a5_integerOffset;

    if (DisplayModeLocal & 1)  // Spinner
      WordIn[4] =  ' ';
    else
      WordIn[4] =  units;

    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (DisplayModeLocal & 1)
      {
        a5loadOSB_Segment (temp1, a5_brightLevel);  // wbp
        if (temp2 > 0)  // wbp
          a5loadOSB_Segment (temp2, a5_brightLevel);  // wbp
        if (units == 'P')
          a5loadOSB_DP("___1_",a5_brightLevel);   // DP dot in DisplayMode 1.        
      }

      if (AlarmIndicate())
        a5loadOSB_DP("2____",a5_brightLevel);     

#ifdef FEATURE_WmGPS
      if (GPSupdating)  // wbp
        a5loadOSB_DP("____2",a5_brightLevel);  // wbp

      if ((DisplayModeLocal < 20) && (DisplayModeLocal & 2) )  // flashing separator?
      { 
        // flashing colon
        if ( (GPS_mode>0) && (abs((now()-tGPSupdate)>300)) )  // if no GPS signal for 5 minutes, alternate flash colon
        {
          if (SecNow & 1)
            a5loadOSB_DP("00200",a5_brightLevel);  // non-flashing separator
          else
            a5loadOSB_DP("01000",a5_brightLevel);  // non-flashing separator
        }
        else if ((SecNow & 1) == 0)  // flash on even seconds
          a5loadOSB_DP("01200",a5_brightLevel);  // non-flashing separator
      }
#else
      if ((DisplayModeLocal < 20) && (DisplayModeLocal & 2) && (SecNow & 1)){ 
        // no HOUR:MINUTE separators
      }
#endif
      else
        a5loadOSB_DP("01200",a5_brightLevel);  // non-flashing separator

      a5BeginFadeToOSB();  
    }  

    MinNowOnesLast = MinNowOnes;  

  }

  else if (DisplayModeLocal == 32)  //Seconds only
  { 

    temp = SecNow;

    SecNowTens = U8DIVBY10(temp);      //i.e.,  SecNowTens = temp / 10;
    SecNowOnes = temp - 10 * SecNowTens;

    WordIn[2] =  SecNowTens + a5_integerOffset;
    WordIn[3] =  SecNowOnes + a5_integerOffset; 
    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmIndicate())
        a5loadOSB_DP("21200",a5_brightLevel);     
      else
        a5loadOSB_DP("01200",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    } 
    SecLast = SecNow; 
  }

  else if (DisplayModeLocal == 33)  //Month, Day
  {  

    if(forceUpdateCopy)
    {  
      temp = day(tNow);  

      byte monthTemp = 3 * ( month(tNow) - 1);  
      //Month name (short):
      //      char a5monthShortNames_P[] PROGMEM = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
      WordIn[0] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp++]));  
      WordIn[1] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp++]));
      WordIn[2] = pgm_read_byte(&(a5_monthShortNames_P[monthTemp]));

      byte divtemp =  U8DIVBY10(temp);  //i.e.,  divtemp = day / 10;

      WordIn[3] =   divtemp + a5_integerOffset;  
      WordIn[4] =  ( temp - 10 * divtemp) + a5_integerOffset;   

      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmIndicate())
        a5loadOSB_DP("20100",a5_brightLevel);     
      else
        a5loadOSB_DP("00100",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }  
    SecLast = SecNow; 
  }
  else if (DisplayModeLocal == 35)  //Year
  { 

    unsigned int yeartemp = year(tNow);  
    unsigned int divtemp =  U16DIVBY10(yeartemp);  //i.e.,  divtemp = yeartemp / 10;

    WordIn[4] =   yeartemp - 10 * divtemp + a5_integerOffset;  
    yeartemp = U16DIVBY10(divtemp); 
    WordIn[3] =   divtemp - 10 * yeartemp + a5_integerOffset;  
    divtemp =  U16DIVBY10(yeartemp); 
    WordIn[2] =   yeartemp - 10 * divtemp + a5_integerOffset;  
    yeartemp = U16DIVBY10(divtemp); 
    WordIn[1] =   divtemp - 10 * yeartemp + a5_integerOffset;  

    if(forceUpdateCopy)
    { 
      a5clearOSB();  
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmIndicate())
        a5loadOSB_DP("20000",a5_brightLevel);     
      else
        a5loadOSB_DP("00000",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }   
  }
  else if (DisplayModeLocal == 36)  //FLW - FIVE LETTER WORD mode
  {

    if(forceUpdateCopy)
    {  
      if (DisplayModeLocalLast != 36)
      {  // Pick new display word, but only when first entering mode 36.

        // Uncomment exactly one of the following two lines:
        FLWoffset = random(fiveLetterWordsMax);  // Random word order!
        // FLWoffset += 1;  // Alphebetical word order!

      }

      if (FLWoffset >= fiveLetterWordsMax)
        FLWoffset = 0;

      unsigned int index = 4 * FLWoffset; 

      WordIn[1] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[2] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[3] = pgm_read_byte(&(fiveLetterWords[index++]));
      WordIn[4] = pgm_read_byte(&(fiveLetterWords[index]));

      temp = 0; 

      while (temp < 25){
        index = pgm_read_word(&(fiveLetterPosArray[temp]));
        if (FLWoffset < index){
          WordIn[0] = 'A' + temp;
          temp = 50;
        } 
        temp++;
      }

      if (temp < 50)
        WordIn[0] = 'Z'; 

      a5clearOSB(); 
      a5loadOSB_Ascii(WordIn,a5_brightLevel);    

      if (AlarmIndicate())
        a5loadOSB_DP("20000",a5_brightLevel);     
      else
        a5loadOSB_DP("00000",a5_brightLevel);     

      a5BeginFadeToOSB(); 
    }   
  }

#ifdef FEATURE_WmGPS
  else if (DisplayModeLocal == 41)  // Time Zone Hour
  {
    unsigned int temp1 = abs(TZ_hour);

    if (TZ_hour < 0)
      WordIn[1] = '-';
    WordIn[2] =  U16DIVBY10(temp1) + a5_integerOffset;
    WordIn[3] =  temp1%10 + a5_integerOffset;

    if(forceUpdateCopy)
    {
      a5clearOSB();
      a5loadOSB_Ascii(WordIn,a5_brightLevel);
      if (AlarmIndicate())
        a5loadOSB_DP("20000",a5_brightLevel);
      else
        a5loadOSB_DP("00000",a5_brightLevel);
      a5BeginFadeToOSB();
    } 

  }

  else if (DisplayModeLocal == 42)  // Time Zone Minutes
  {

    WordIn[2] =  uint8_t(TZ_minutes/10) + a5_integerOffset;
    WordIn[3] =  TZ_minutes%10 + a5_integerOffset;

    if(forceUpdateCopy)
    {
      a5clearOSB();
      a5loadOSB_Ascii(WordIn,a5_brightLevel);
      if (AlarmIndicate())
        a5loadOSB_DP("20000",a5_brightLevel);
      else
        a5loadOSB_DP("00000",a5_brightLevel);
      a5BeginFadeToOSB();
    }

  }
#endif

  DisplayModeLocalLast = DisplayModeLocal;
  SecLast = SecNow; 

}


void SerialPrintTime(){
  //   Print time over serial interface.   Adapted from Time library.

	time_t timeTmp = now();  

  Serial.print(hour(timeTmp));
  printDigits(minute(timeTmp));
  printDigits(second(timeTmp));
  Serial.print(" ");
  Serial.print(dayStr(weekday(timeTmp)));
  Serial.print(" ");
  Serial.print(day(timeTmp));
  Serial.print(" ");
  Serial.print(monthShortStr(month(timeTmp)));
  Serial.print(" ");
  Serial.print(year(timeTmp)); 
  Serial.println(); 

}

void printDigits(int digits){
  // utility function for digital clock serial output: prints preceding colon and leading 0
  // borrowed from Time library.
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void ApplyDefaults (void) {
  // VARIABLES THAT HAVE EEPROM STORAGE AND DEFAULTS...

  a5_brightLevel =  a5brightLevelDefault;
  HourMode24 =      a5HourMode24Default;
  AlarmEnabled =    a5AlarmEnabledDefault;
  AlarmTimeHr =     a5AlarmHrDefault;
  AlarmTimeMin =    a5AlarmMinDefault;
  AlarmTone =       a5AlarmToneDefault;
  NightLightType =  a5NightLightTypeDefault;  
  numberCharSet =   a5NumberCharSetDefault;
#ifdef FEATURE_AUTODST
  DST_mode =        a5GPSModeDefault;
#endif
#ifdef FEATURE_WmGPS
  GPS_mode =        a5GPSModeDefault;
  TZ_hour =         a5TZHourDefault;
  TZ_minutes =      a5TZMinutesDefault;
#endif

}



void EEReadSettings (void) {  
  // Check values for sanity at THIS stage.
  byte value = 255;
  value = EEPROM.read(0);      

  if ((value > 100 + BrightnessMax) || (value < 100))
    Brightness = a5brightLevelDefault;
  else  
    Brightness = value - 100;   

  value = EEPROM.read(1);
  if (value > 1)
    HourMode24 = a5HourMode24Default;
  else  
    HourMode24 = value;

  value = EEPROM.read(2);
  if (value > 1)
    AlarmEnabled = a5AlarmEnabledDefault;
  else  
    AlarmEnabled = value;

  value = EEPROM.read(3); 
  if ((value > 123) || (value < 100))
    AlarmTimeHr = a5AlarmHrDefault;
  else  
    AlarmTimeHr = value - 100;   

  value = EEPROM.read(4);
  if ((value > 159) || (value < 100))
    AlarmTimeMin = a5AlarmMinDefault;
  else  
    AlarmTimeMin = value - 100;   

  value = EEPROM.read(5);
  if (value > 5)
    AlarmTone = a5AlarmToneDefault;
  else  
    AlarmTone = value;   

  value = EEPROM.read(6);
  if (value > 4)
    NightLightType = a5NightLightTypeDefault;  
  else  
    NightLightType = value;    

  value = EEPROM.read(7);
  if (value > 9)
    numberCharSet = a5NumberCharSetDefault;
  else
    numberCharSet = value;

  value = EEPROM.read(8);
  if (value > 31)
    DisplayMode = a5DisplayModeDefault;
  else
    DisplayMode = value;

#ifdef FEATURE_AUTODST
  value = EEPROM.read(9);
  if (value > 2)
    DST_mode = a5DSTModeDefault;
  else
    DST_mode = value;
#endif

#ifdef FEATURE_WmGPS
  value = EEPROM.read(10);
  if (value > 2)
    GPS_mode = a5GPSModeDefault;
  else  
    GPS_mode = value;

  value = EEPROM.read(11);   // TZ_hour
  if ((value > 24) || (value<0))
    TZ_hour = a5TZHourDefault;
  else
    TZ_hour = value-12;

  value = EEPROM.read(12);   // TZ_minutes
  if ((value > 45) || (value < 0))
    TZ_hour = a5TZMinutesDefault; 
  else
    TZ_minutes = value;
#endif

}

void EESaveSettings (void){ 

  // If > 4 seconds since last button press, and
  // we suspect that we need to change the stored settings:

  byte value; 
  byte indicateEEPROMwritten = 0;

  if (milliTemp >= (LastButtonPress + 4000))
  {

    // Careful if you use this function: EEPROM has a limited number of write
    // cycles in its life.  Good for human-operated buttons, bad for automation.
    // Also, no error checking is provided at this, the write EEPROM stage.

    value = EEPROM.read(0);  
    if (Brightness != (value - 100))  {
      a5writeEEPROM(0, Brightness + 100);  
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(1);  
    if (HourMode24 != value)  { 
      a5writeEEPROM(1, HourMode24); 
      indicateEEPROMwritten = 1;
    } 
    value = EEPROM.read(2);  
    if (AlarmEnabled != value)  {
      a5writeEEPROM(2, AlarmEnabled);  
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    } 
    value = EEPROM.read(3);  
    if (AlarmTimeHr != (value - 100))  { 
      a5writeEEPROM(3, AlarmTimeHr + 100); 
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(4);  
    if (AlarmTimeMin != (value - 100)){
      a5writeEEPROM(4, AlarmTimeMin + 100); 
      //NOTE:  Do not blink LEDs off to indicate saving of this value
    }
    value = EEPROM.read(5);  
    if (AlarmTone != value){ 
      a5writeEEPROM(5, AlarmTone);
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(6);  
    if (NightLightType != value){
      a5writeEEPROM(6, NightLightType);  
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(7);  
    if (numberCharSet != value){
      a5writeEEPROM(7, numberCharSet);  
      indicateEEPROMwritten = 1;
    }
    value = EEPROM.read(8);  
    if (DisplayMode != value){
      a5writeEEPROM(8, DisplayMode);  
      indicateEEPROMwritten = 1;
    }      
#ifdef FEATURE_AUTODST
    value = EEPROM.read(9);  
    if (DST_mode != value){
      a5writeEEPROM(9, DST_mode);  
      indicateEEPROMwritten = 1;
    }      
#endif
#ifdef FEATURE_WmGPS
    value = EEPROM.read(10);  
    if (GPS_mode != value){
      a5writeEEPROM(10, GPS_mode);  
      indicateEEPROMwritten = 1;
    }      
    value = EEPROM.read(11);  
    if ((TZ_hour+12) != value){
      a5writeEEPROM(11, TZ_hour+12);  
      indicateEEPROMwritten = 1;
    }      
    value = EEPROM.read(12);  
    if (TZ_minutes != value){
      a5writeEEPROM(12, TZ_minutes);  
      indicateEEPROMwritten = 1;
    }
#endif

    if (indicateEEPROMwritten) { // Blink LEDs off to indicate when we're writing to the EEPROM 
      DisplayWord ("SAVED", 500);  
    }

    UpdateEE = 0;
    if (UseRTC)  
      RTC.set(now());  // Update time at RTC, in case time was changed in settings menu
  }
} 


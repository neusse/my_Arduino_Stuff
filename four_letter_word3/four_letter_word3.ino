`/* four letter word generator

version 0.1
Author: Alexander Davis
Date: 04/12/2011
Hardware: pretty much any arduino

Creates four-letter words on the fly

Sends output to the serial port

*/

// nicer string handling
#include <string.h>
// serial port speed
#define SERIAL_BAUD 9600
// delay between strings in milliseconds
#define INTER_DELAY 0
#define SCROLL_DELAY 200
#define BLANK_DELAY 200

String d1 = "NONAKISNBLCLWRSTFRSHCRPISUFULUZAZOZICOCAMADEBAPABOHAMOLABURATAMIHOPODITOMENEPEPOPASOSASESIGRGOGAWAWOWISLPLFIPIJAJOJEFEBRTRQU";
String d2 = "ONOMUNXYESTSNSLLDSCKREGSPSSTEDBSNELENGKEMSATTEWSNTARSHNKND";
String ts1,
       ts2,
       ts3;
char msg0[40] = "FOUR LETTER WORD GENERATOR (C) AWD 2011";
char msg1[46] = "AND NOW FOR SOMETHING COMPLETELY DIFFERENT...";
     
int p1,
    p2;

int wordDelay = 750;
    
char buffer[5];

int whatever,
    count = 0;
    
// for tracking time in delay loops
unsigned long previousMillis;

void setup()
{
  //Serial.begin(SERIAL_BAUD);
  randomSeed(analogRead(1));

  byte pinLoop;
  // set port d to all outputs, except serial in
  // then set them low
  DDRD = B11111110;
  PORTD = B00000000;
  // set pins 8-14 to outputs
  for (pinLoop = 8; pinLoop < 15; pinLoop++) {
    pinMode(pinLoop, OUTPUT);
  }

  // pins 18 & 19 use for i2c for RTC
  
  // set the pin states
  // use 8 for BL and set to HIGH
  digitalWrite(8, HIGH);
  // use 9 for AO and set to LOW
  digitalWrite(9, LOW);
  // use 10 for A1 and set to LOW
  digitalWrite(10, LOW);

  // if you tie C1 and C2 on each display
  // then 11 can be for display0
  // and 12 can be for display1

  // use 11 for C1 and set to HIGH
  digitalWrite(11, HIGH);
  // use 12 for C2 and set to HIGH
  digitalWrite(12, HIGH);
  // use 13 for CLR and set to HIGH
  digitalWrite(13, HIGH);
  // use 14 for WR and set to HIGH
  digitalWrite(14, HIGH);
  
  // print intro message
  whatever = formatStr(msg0);
  delay(3000);
  wordDelay = 5000;
}

void loop()
{
  p1 = 2 * int(random(55));
  p2 = p1 + 2;
  ts1 = d1.substring(p1, p2);
  p1 = 2 * int(random(28));
  p2 = p1 + 2;
  ts2 = d2.substring(p1, p2);
  ts3 = ts1 + ts2;
  // word must contain at least one vowel
  if ((ts3.indexOf('A') > -1) || (ts3.indexOf('E') > -1) || (ts3.indexOf('I') > -1) || (ts3.indexOf('O') > -1) || (ts3.indexOf('U') > -1))
  {
    // the vowel must be in the first three characters
    if ((ts3.substring(1, 3).indexOf('A') > -1) || (ts3.substring(1, 3).indexOf('E') > -1) || (ts3.substring(1, 3).indexOf('I') > -1) || (ts3.substring(1, 3).indexOf('O') > -1) || (ts3.substring(1, 3).indexOf('U') > -1))
    {
      // display the string
      ts3.toCharArray(buffer, 5);
      //Serial.println(buffer);
      if (count == 100)
      {
        wordDelay = 750;
        whatever = formatStr(msg1);
        delay(3000);
        count = 0;
        wordDelay = 5000;
      }
      else
      {
        whatever = formatStr(buffer);
        count++;
      }
      previousMillis = millis();
      // loop until now - previous is bigger than the inter-string delay time
      // break out of the loop if a button is pressed
      // buttonCtrl() returns -1,0,1 or 2 if a button was pressed
      // otherwise it returns 3
      while (millis() - previousMillis < INTER_DELAY)
      {
        // do nothing
      }
    }
  }
}
/* ------------------------------------------------------------
   function formatStr
   purpose: formats a string for display on a DL2416T
   expects: a pointer to a string
   returns: an int status for button control:
            -1 if back
            1 if forward
            3 if pause/resume
   ------------------------------------------------------------ */

int formatStr(char toDisplay[])
{
  int wStart = 0, // start of word in string
  wEnd = 0; // end of word in string
  int count = 0, // string position counter
  x = 0, // string position counter for word < 5
  n = 0, // display counter for word > 5 mid-word
  wordLen = 0, // length of string
  d = 0, // display counter for word >5 end word
  dLen = 0, // length - 1 for display purposes
  offset = 0, // an offset to center the word on the display
  buttonResult = 3; // button status from display function
  byte displayNum; // selects display 0 or 1

  byte setChar;
  int segCount = 0;

  // loop until the end of the string

  while (toDisplay[count] != '\0')
  {
    // assume string begins a word and not with a space
    wStart = count;
    wordLen = 0;


    // find the length of the word until we reach a space
    // or the end of the string
    while ((toDisplay[count] != ' ') && (toDisplay[count] != '\0'))
    {
	// set the end to the current location
	// when we drop out of the loop this will be
	// the last character in the word
	wEnd = count;
	// move to the next position in the string
	count++;
	// increment the character count
	wordLen++;
    }
    
    // start with 1st display, displayNum = 0
    displayNum = 0;

    // display directly if it fits on the display
    if (wordLen < 5)
    {
	// write the characters between wStart and wEnd directly
	// to the display

	// back through the buffer array and set one character
	// on the display at a time

	// blank the display
	digitalWrite(8, LOW);
	//delay(1);

	// go through the buffer array backwards and write the
	// characters to the display
	segCount = 0;      
        if (wordLen == 2)
        {
          offset = 1;
        }
        if (wordLen == 1)
        {
          offset = 2;
        }        
        // apply the offset to the segCount
        segCount = segCount + offset;

	for (n = wEnd; n > (wStart - 1); n--)
	{
          // move to 2nd display if segCount > 3
          // eg. displayNum = 1
          if (segCount > 3)
          {
            displayNum = 1;
          }
	  // send the character, display position and display
          // get back the button pressed code, if any
	  buttonResult = displayChar(toDisplay[n], segCount, displayNum);
	  segCount++;
          // if any button is pressed return to the calling function
          // with the button pressed code
          if (buttonResult != 3)
          {
            return buttonResult;
          }

	}
	// unblank the display
	digitalWrite(8, HIGH);

	// a pause to read the word
        // grab 'start' in millis
        previousMillis = millis();
        // loop until now - previous is bigger than the inter-string delay time
        // break out of the loop if a button is pressed
        while (millis() - previousMillis < wordDelay)
        {
          // if any button is pressed return to the calling function
          // with the button pressed code
          if (buttonResult != 3)
          {
            return buttonResult;
          }
        }

	// clear the display
	digitalWrite(13, LOW);
	digitalWrite(13, HIGH);

        // small pause
        delay(100);

        // reset offset
        offset = 0;
        
    }

    // if it is more than eight characters long
    // display using the scrolling method
    if (wordLen > 4)
    {
	dLen = wordLen - 1;

	// write the characters between wStart and wEnd so they
	// appear to scroll from left to right

	// it takes length + 2 iterations to scroll
	// all the way to the last character

	// we will use x to keep track of where we are in the
	// scrolling process
	for (x = 0; x < (wordLen + 3); x++)
	{

	  // clear the display
	  digitalWrite(13, LOW);
	  digitalWrite(13, HIGH);

	  // scroll until the eighth character in the word
	  // read the buffer backwards
	  if (x < 4) {
            // start with the first display
            displayNum = 0;

	    segCount = 0;
	    for (d = (wEnd - (dLen - x)); d > (wStart - 1) ; d--)
	    {
                
		// send the character, display position and display
                // get back the button pressed code, if any
		buttonResult = displayChar(toDisplay[d], segCount, displayNum);
		segCount++;
                // if any button is pressed return to the calling function
                // with the button pressed code
                if (buttonResult != 3)
                {
                  return buttonResult;
                }
	    }
	  }
	  // after the eighth character scroll on
	  // until the end of the word
	  if ((x > 3) && (x < wordLen))
	  {
	    segCount = 0;
            displayNum = 0;

	    for (d = wEnd - (dLen - x); d > ((wStart + (x - 3)) - 1); d--)
	    {
                
		// send the character, display position and display
		buttonResult = displayChar(toDisplay[d], segCount, displayNum);
		segCount++;
                // if any button is pressed return to the calling function
                // with the button pressed code
                if (buttonResult != 3)
                {
                  return buttonResult;
                }      
	    }
	  }

	  // once we get to the end of the word
	  // scroll off by reading forwards in the array
	  // while counting backwards in the display position
	  if (x > dLen)
	  {
	    segCount = 3;
            displayNum = 0;
	    for (d = (wStart + (x - 3)); d < (wEnd + 1); d++)
	    {
		// send the character, display position and display
		buttonResult = displayChar(toDisplay[d], segCount, displayNum);
		segCount--;
                // if any button is pressed return to the calling function
                // with the button pressed code
                if (buttonResult != 3)
                {
                  return buttonResult;
                }
	    }
	  }
	// a small delay between scroll
        // grab 'start' in millis
        previousMillis = millis();
        // loop until now - previous is bigger than the inter-string delay time
        // break out of the loop if a button is pressed
        while (millis() - previousMillis < SCROLL_DELAY)
        {
          // if any button is pressed return to the calling function
          // with the button pressed code
          if (buttonResult != 3)
          {
            return buttonResult;
          }
        }
      }
      // clear the display
      digitalWrite(13, LOW);
      digitalWrite(13, HIGH);
    }
    if (toDisplay[count] != '\0')
    {
      count++;
    }
  }
  return buttonResult;
} // end function formatStr


/* ------------------------------------------------------------
   function displayChar
   purpose: displays a character on a DL2416T
   expects: a character, a position and a display (0,1,2 etc...)
   returns: an int for button control:
            -1 if back
            1 if forward
            2 if pause/resume
   ------------------------------------------------------------ */

int displayChar(char myChar, int myPos, byte myDisp)
{

  /* here's how to set a character:
	   1. select an address using a0-a1 which here are arduino pins 9-10
	   2. pull WR to LOW to enable loading
	   3. send the bit-shifted character to the data lines d0-d6 all at once using PORTD
	   4. set the WR pin back to HIGH
	   5. repeat for next character

   */

  // enable the correct display
  if (myDisp == 0)
  {
    // enable display 0 by setting C1-C2 LOW
    digitalWrite(11, LOW);
    // disable display 1 by setting C1-C2 HIGH
    digitalWrite(12, HIGH);

  }
  if (myDisp == 1)
  {
     // enable display 1 by setting C1-C2 LOW
    digitalWrite(12, LOW);
    // disable display 0 by setting C1-C2 HIGH
    digitalWrite(11, HIGH);
    
    // convert the 7-0 word frame position to the
    // actual position on display 1
    myPos = myPos - 4;
  }

  // we are going to set the ASCII character all at once on PORTD
  // but we must avoid pin 0, which is serial RX
  // so we will shift one bit to the left
  myChar = myChar << 1;
  // the segment position is the opposite of the array
  // eg. seg0 is array[3]
  // set the character pin state
  // cheap binary conversion of setChar
  switch (myPos)
  {
    case 0:
	digitalWrite(9, LOW);
	digitalWrite(10, LOW);
	break;
    case 1:
	digitalWrite(9, HIGH);
	digitalWrite(10, LOW);
	break;
    case 2:
	digitalWrite(9, LOW);
	digitalWrite(10, HIGH);
	break;
    case 3:
	digitalWrite(9, HIGH);
	digitalWrite(10, HIGH);
	break;
  }

  // set the WR pin LOW to enable loading
  digitalWrite(14, LOW);

  // set the shifted character to PORTD
  PORTD = myChar;

  // set WR pin HIGH to finish loading
  digitalWrite(14, HIGH);
  
  // check the buttons and return the button pressed
  // or not pressed code
  return 3;
} // end function displayChar

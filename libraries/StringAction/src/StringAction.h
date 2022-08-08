/*
    StringAction.cpp - Library for processing strings and reading from serial.
    Created by Vsevolod Kabrits, June 26, 2021. Version 1.0.5.
    Released into the public domain.
*/

#ifndef StringAction_h
#define StringAction_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SoftwareSerial.h"

class StringAction
{
  public:

    void rs(String & istr, int wait_delay = 500);
    // reads string from serial
    // @parameter: istr -- string to write read data to
    // @parameter: wait_delay -- integer delay in ms after wich reading stops
    
    void rs(String & istr, volatile bool & inter, int wait_delay = 500);
    // reads string from serial
    // @parameter: istr -- string to write read data to
    // @parameter: inter -- interrupt variable, if equals 1 waiting for the data ends
    // @parameter: wait_delay -- integer delay in ms after wich reading stops
    
    void rs(String & istr, SoftwareSerial port, int wait_delay = 500);
    // reads string from serial
    // @parameter: istr -- string to write read data to
    // @parameter: port -- software serial port to read from
    // @parameter: wait_delay -- integer delay in ms after which reading stops
    
    void rs(String & istr, SoftwareSerial port, volatile bool & inter, int wait_delay = 500);
    // reads string from serial
    // @parameter: istr -- string to write read data to
    // @parameter: port -- software serial port to read from
    // @parameter: inter -- interrupt variable, if equals 1 waiting for the data ends
    // @parameter: wait_delay -- integer delay in ms after which reading stops

    void split (String s, String split_arr[], String delimeter, int max_length = -1);
    // splits string to string array by delimeter
    // @parameter: s -- string to split
    // @parameter: split_arr -- array to which splited strings are written
    // @parameter: delimeter -- string by which s is splitted
    // @parameter: max_length -- integer length of split_arr, if default, writes to split array until string has parts separated by delimeter
  
};

#endif

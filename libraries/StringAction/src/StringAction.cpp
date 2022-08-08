/*
    StringAction.cpp - Library for processing strings and reading from serial.
    Created by Vsevolod Kabrits, June 26, 2021. Version 1.0.5.
    Released into the public domain.
*/

#include <StringAction.h>
#include <SoftwareSerial.h>


void StringAction::rs(String & istr, int wait_delay){
  String b = "";
  char a = '\0';
  unsigned long cms = millis();
    
  while (a != '\n' && abs(millis() - cms) < wait_delay){
    if (Serial.available()){
      b += String(a);
      a = Serial.read();
    }
  }
    
  b.trim();
  istr = b;
}

void StringAction::rs(String & istr, volatile bool & inter, int wait_delay){
  String b = "";
  char a = '\0';
  unsigned long cms = millis();
    
  while (a != '\n' && abs(millis() - cms) < wait_delay){
    if (Serial.available()){
      b += String(a);
      a = Serial.read();
    }
    if (inter){
      break;
    }
  }
    
  b.trim();
  istr = b;
}

void StringAction::rs(String & istr, SoftwareSerial port, int wait_delay){
    String b = "";
    char a = '\0';
    unsigned long cms = millis();
    while (a != '\n' && abs(millis() - cms) < wait_delay){
      if (Serial.available()){
        b += String(a);
        a = port.read();
      }
    }
    b.trim();
    istr = b;
}

void StringAction::rs(String & istr, SoftwareSerial port, volatile bool & inter, int wait_delay){
    String b = "";
    char a = '\0';
    unsigned long cms = millis();
    while (a != '\n' && abs(millis() - cms) < wait_delay){
      if (Serial.available()){
        b += String(a);
        a = port.read();
      }
      if (inter){
        break;
      }
    }
    b.trim();
    istr = b;
}

void StringAction::split (String s, String split_arr[], String delimeter, int max_length) {
  int i = 0;
  int a = s.indexOf(delimeter);
  while (a != -1) {
    split_arr[i] = s.substring(0, a);
    if (a+1 > s.length()-1) {
      break;
    }
    s = s.substring(a+1);
    a = s.indexOf(delimeter);
    i++;
    if (max_length != -1 and i >= max_length) {
        break;
    }
  }
  if (s.length() != 0) {
    split_arr[i] = s;
  }
}

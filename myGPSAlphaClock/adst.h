/*
 * Auto DST support for VFD Modular Clock
 * (C) 2012 William B Phelps
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#ifndef ADST_H_
#define ADST_H_

#include "Time.h"
#include "Arduino.h"

#define FEATURE_WmGPS
#define FEATURE_AUTO_DST

extern int8_t DST_mode;  // DST off, on, auto?
extern uint8_t DST_offset;  // DST offset in Hours
extern int8_t DST_updated;  // DST update flag = allow update only once per day
//DST_Rules dst_rules = {{3,1,2,2},{11,1,1,2},1};   // initial values from US DST rules as of 2011
extern int8_t DST_Rules[9];

char* dst_setting(byte dst);
byte dotw(unsigned int year, byte month, byte day);
//void DSTinit(tmElements_t* te, DST_Rules* rules);
//uint8_t getDSToffset(tmElements_t* te, DST_Rules* rules);

//void DSTinit(tmElements_t* te, int8_t rules[9]);
//byte getDSToffset(tmElements_t* te, int8_t rules[9]);
void DSTinit(time_t tm, int8_t rules[9]);
byte getDSToffset(time_t tm, int8_t rules[9]);
void setDSToffset(uint8_t mode);

#endif

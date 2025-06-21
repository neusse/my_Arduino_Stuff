#include <Arduino.h>
#include "constants.h"
#include "ansi.h"

void debug(const char *string) {
  #ifdef DEBUG
  ansi_debug_ln(string);
  //delay(1000);
  #endif
}

void debug(const __FlashStringHelper *string) {
  #ifdef DEBUG
  ansi_debug_ln(string);
  //delay(1000);
  #endif
}
#include <StringAction.h>
String m, split_arr[10];
StringAction s;
void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    s.rs(m);
    s.split(m, split_arr, " ");
    for (int i = 0; i < 10; i++) {
        Serial.println(split_arr[i]);
        split_arr[i] = "";
    }
  }
}

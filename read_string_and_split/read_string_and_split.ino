#include <StringAction.h>
String m, split_arr[10];
StringAction s;
void setup() {
  Serial.begin(19200);
}

void loop() {

    s.split("this,is,a,test,,,yes123,dd,dd", split_arr, ",");
    for (int i = 0; i < 10; i++) {
        Serial.println(split_arr[i]);
        split_arr[i] = "";
    }
 }

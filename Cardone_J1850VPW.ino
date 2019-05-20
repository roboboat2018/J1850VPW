#include"Cardone_j1850vpw.h"

j1850 j;

void setup() {
Serial.begin(115200);
Serial.println("starting j1850vpw....");
j.init(8, 9);


}

void loop() {
  byte rx_buf[12]; 
   //j.j1850vpw_send(6, 0x48, 0x69, 0x2D, 0x4D, 0x32, 0x21);    
   //delay(10); 
 if (j.accept(rx_buf)) {                                       
}
}

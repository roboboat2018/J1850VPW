#include"Cardone_j1850vpw.h"

j1850 j;

void setup() {
Serial.begin(115200);
Serial.println("j1850vpw starting...");

j.init(8, 9, 1);
 

}

void loop() {
  byte rx_buf[12];      
   if (j.accept(rx_buf)) {                                    
        //j.j1850vpw_send(6, 0x48, 0x69, 0x2D, 0x4D, 0x32, 0x21);   
}
}

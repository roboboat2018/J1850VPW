#include"Cardone_j1850vpw.h"
#include <Arduino.h>

void j1850:: wait_idle(void)    { 
   start_timer();
 while (read_timer() < RX_IFS_MIN/0.0625) {
    if (is_active())
      start_timer();
  }
  }
void j1850::Cardone_j1850vpw_ouput(int output_pin, bool state){

if(state){
digitalWrite(output_pin,HIGH);
pinMode(output_pin,OUTPUT);
}
else{
digitalWrite(output_pin,LOW);
pinMode(output_pin,INPUT);

}
}

void j1850::init(int in_pin_, int out_pin_) {
  out_pin = out_pin_;
  in_pin = in_pin_;
  digitalWrite(in_pin,HIGH);
  pinMode(in_pin,INPUT);
  passive();
  TCCR1B=1<<(CS10); 
  if_init = true;
 
}
bool j1850::is_active(void) {
  return digitalRead(in_pin);
}
void j1850::start_timer() { 
  TCCR1A=0; 
  TCNT1=0;  
  }
unsigned int j1850::read_timer(void) {
  return TCNT1; }
void j1850::active(void) {
  Cardone_j1850vpw_ouput(out_pin,HIGH);
}

void j1850::passive(void) {
  Cardone_j1850vpw_ouput(out_pin,LOW);
}
bool j1850::j1850vpw_send(int size, ...) {
  if (size == 0)
    return false;
  byte *buffer = new byte[size];
  bool result = false;
  va_list ap;

  va_start(ap, size);

  for (int i = 0; i < size; i++)
    buffer[i] = va_arg(ap, int);

  va_end(ap);

  result = send(buffer, size);
  delete[] buffer;
  return result;
}
bool j1850::send(byte *msg_buf, int nbytes) {
  if (!if_init)
    return false;
  msg_buf[nbytes] = crc(msg_buf, nbytes);
  nbytes++;
  bool f = send_msg(msg_buf, nbytes);
  monitor();
  return f;
}
bool j1850::send_msg(byte *msg_buf, int nbytes) {
  int nbits;
  byte temp_byte;
  tx_msg_buf = msg_buf;
  tx_nbyte = nbytes;
  if (nbytes > 12) {
    message = ERROR_MESSAGE_TOO_LONG;
    return false;
  }
wait_idle();
active();
start_timer();
while(read_timer()<TX_SOF/0.0625);
 
do
  {
    temp_byte = *msg_buf; // store byte temporary
    nbits = 8;
    while (nbits--)   // send 8 bits
    { 
      if(nbits & 1) // start allways with passive symbol
      { start_timer();
        delay = (temp_byte & 0x80) ? TX_LONG : TX_SHORT;  // send correct pulse lenght 
        passive();  // set bus passive    
        while (read_timer() <= delay/0.0625-40/0.0625)  // wait
        {
          if(is_active())  // check for bus error
          {  message=RETURN_CODE_BUS_ERROR;
             return false ; // error, bus collision!
          }
        }
      }
      else  // send active symbol
      {
        start_timer();
        delay = (temp_byte & 0x80) ? TX_SHORT : TX_LONG;  // send correct pulse lenght
        active(); // set bus active 
        while (read_timer() <= delay/0.0625); // wait
        // no error check needed, ACTIVE dominates
      }
      temp_byte <<= 1;  // next bit
    }// end nbits while loop
    ++msg_buf;  // next byte from buffer
  } while(--nbytes);// end nbytes do loop
   
  passive();  // send EOF symbol
  start_timer();
  while (read_timer() <= TX_EOF/0.0625); // wait for EOF complete
  message=MESSAGE_SEND_OK;
  return true;  // no error
}
bool j1850::recv_msg(byte *msg_buf) {
  int nbits, nbytes;
  bool bit_state;
  rx_msg_buf = msg_buf;
  
  start_timer();
  while (!is_active()) {
    if (read_timer() > WAIT_100us/0.0625) {
      message = ERROR_NO_RESPONDS_WITHIN_100US;
      return false;
    }
  }

  start_timer();
  while (is_active()) {
    if (read_timer() > RX_SOF_MAX/0.0625) {
      message = ERROR_ON_SOF_TIMEOUT;
      return false;
    }
  }

  if (read_timer() < RX_SOF_MIN/0.0625) {
    message = ERROR_SYMBOL_WAS_NOT_SOF;
    return false;
  }

  bit_state = is_active();
  start_timer();

  for (nbytes = 0; nbytes < 12; ++nbytes) {
    nbits = 8;
    do {
      *msg_buf <<= 1;
      while (is_active() == bit_state) {
        if (read_timer() > RX_EOD_MIN/0.0625) {
          rx_nbyte = nbytes;
          message = MESSAGE_ACCEPT_OK;
          return true;
        }
      }

      bit_state = is_active();
      long tcnt1_buf = read_timer();
      start_timer();

      if (tcnt1_buf < RX_SHORT_MIN/0.0625) {
        message = ERROR_SYMBOL_WAS_TOO_SHORT;
        return false;
      }

      if ((tcnt1_buf < RX_SHORT_MAX/0.0625) && !is_active())
        *msg_buf |= 1;

      if ((tcnt1_buf > RX_LONG_MIN/0.0625) && (tcnt1_buf < RX_LONG_MAX/0.0625) && is_active())
        *msg_buf |= 1;

    } while (--nbits);
    ++msg_buf;
  }

  rx_nbyte = nbytes;
  message = MESSAGE_ACCEPT_OK;
  return true;
}
bool j1850::accept(byte *msg_buf) {
  if (!if_init)
    return false;
  bool f = recv_msg(msg_buf);

  if (f) {
    if (msg_buf[rx_nbyte - 1] != crc(msg_buf, rx_nbyte - 1)) {
      f = false;
      message = ERROR_ACCEPT_CRC;
    }else{
      message=MESSAGE_ACCEPT_OK;
      monitor();
    }
    
  }
  
  return f;
}
void j1850::monitor(void) {
  static int old_message;
  if (MESSAGE_SEND_OK == message)
        sendToUART("TX: ", tx_nbyte, tx_msg_buf);
  if (MESSAGE_ACCEPT_OK == message)
        sendToUART("RX: ", rx_nbyte, rx_msg_buf);
}
void j1850::sendToUART(const char *header, int rx_nbyte, byte *msg_buf) {
  Serial.print(header);
  for (int i = 0; i < rx_nbyte; i++) {
    if (msg_buf[i] < 0x10)
      Serial.print(0);

    Serial.print(msg_buf[i], HEX);

    if (i == (rx_nbyte - 1)) {
      Serial.print("\n");
    } else {
      Serial.print(" ");
    }
  }
}

byte j1850::crc(byte *msg_buf, int nbytes) {
  byte crc = 0xFF;
  while (nbytes--) {
    crc ^= *msg_buf++;
    for (int i = 0; i < 8; i++)
      crc = crc & 0x80 ? (crc << 1) ^ 0x1D : crc << 1;
  }
  crc ^= 0xFF;
  return crc;
}

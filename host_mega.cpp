#ifdef __AVR_ATmega2560__

#include <Arduino.h>
#include "Altair8800.h"
#include "config.h"
#include "mem.h"
#include "host_mega.h"

#if NUM_DRIVES>0
#error Arduino MEGA port does not support disk drives. Set NUM_DRIVES to 0 in config.h
#endif

#if USE_THROTTLE>0
#error Throttling neither supported nor necessary for Arduino MEGA. Set USE_THROTTLE to 0 in config.h
#endif


/*
  Runs emulation at about 0.5 Mhz clock speed (about 1/4 speed of original Altair8800)

  Function switch pin mapping (digital input):
     RUN          => 20
     STOP         => 21 (Port D, bit 0)
     STEP         =>  4
     SLOW         =>  5
     EXAMINE      =>  6
     EXAMINE NEXT =>  7
     DEPOSIT      =>  8
     DEPOSIT NEXT =>  9
     RESET        => 18 (Port D, bit 3)
     CLR          => 19 (Port D, bit 2)
     PROTECT      => 16
     UNPROTECT    => 17
     AUX1 UP      => 14
     AUX1 DOWN    => 15
     AUX2 UP      =>  3 (PORT E, bit 5)
     AUX2 DOWN    =>  2 (PORT E, bit 4)

   Address/Data switch pin mapping (analog input):
     A0...15      => A0...15

   Status LED mapping (digital output):
    +A0..7        => 22, 23, ..., 29  (PORTA)
    +A8..15       => 37, 36, ..., 30  (PORTC)
    +D0..8        => 49, 48, ..., 42  (PORTL)
    *INT          => 53 \
     WO           => 52 |
     STACK        => 51 |
     HLTA         => 50 | STATUS      (PORTB)
     OUT          => 10 |
     M1           => 11 |
     INP          => 12 |
     MEMR         => 13 /
     INTE         => 38
     PROT         => 39
     WAIT         => 40
    *HLDA         => 41

*/


uint16_t host_read_status_leds()
{
  uint16_t res = PORTB;
  res |= PORTD & 0x80 ? ST_INTE : 0;
  res |= PORTG & 0x04 ? ST_PROT : 0;
  res |= PORTG & 0x02 ? ST_WAIT : 0;
  res |= PORTG & 0x01 ? ST_HLDA : 0;
  return res;
}


//------------------------------------------------------------------------------------------------------


void host_copy_flash_to_ram(void *dst, const void *src, uint32_t len)
{
  for(uint32_t i=0; i<len; i++) 
    ((byte *) dst)[i] = pgm_read_byte(((byte *) src)+i);
}


void host_write_data(const void *data, uint32_t addr, uint32_t len)
{
  byte *b = (byte *) data;
  for(uint32_t i=0; i<len; i++) EEPROM.write(addr+i, b[i]);
}


void host_read_data(void *data, uint32_t addr, uint32_t len)
{
  byte *b = (byte *) data;
  for(uint32_t i=0; i<len; i++) b[i] = EEPROM.read(addr+i);
}


void host_move_data(uint32_t to, uint32_t from, uint32_t len)
{
  uint32_t i;
  if( from<to )
    {
      for(i=0; i<len; i++) EEPROM.write(to+len-i-1, EEPROM.read(from+len-i-1));
    }
  else
    {
      for(i=0; i<len; i++) EEPROM.write(to+i, EEPROM.read(from+i));
    }
}


// --------------------------------------------------------------------------------------------------


uint32_t host_get_random()
{
  return (((uint32_t) random(0,65535)) * 65536l | random(0,65535));
}


// --------------------------------------------------------------------------------------------------

volatile static uint16_t switches_pulse = 0;
volatile static uint16_t switches_debounced = 0;
static uint32_t debounceTime[16];
static const byte function_switch_pin[16] = {20, 21, 4, 5, 6, 7, 8, 9, 18, 19, 16, 17, 14, 15, 3, 2};
static const uint16_t function_switch_irq[16] = {0, INT_SW_STOP, 0, 0, 0, 0, 0, 0, INT_SW_RESET, INT_SW_CLR, 
                                                 0, 0, 0, 0, INT_SW_AUX2UP, INT_SW_AUX2DOWN};


static void switch_check(byte i)
{
  if( millis()>debounceTime[i] )
    {
      uint16_t bitval = 1<<i;
      bool d1 = !digitalRead(function_switch_pin[i]);
      bool d2 = (switches_debounced & bitval) ? true : false;

      if( d1 && !d2 ) 
        {
          switches_debounced |= bitval;
          switches_pulse     |= bitval;
          if( function_switch_irq[i] ) altair_interrupt(function_switch_irq[i]);
          debounceTime[i] = millis() + 100;
        }
      else if( !d1 && d2 ) 
        {
          switches_debounced &= ~bitval;
          switches_pulse     &= ~bitval;
          debounceTime[i] = millis() + 100;
        }
    }
}


bool host_read_function_switch(byte i)
{
  return !digitalRead(function_switch_pin[i]);
}


bool host_read_function_switch_debounced(byte i)
{
  if( function_switch_irq[i]==0 ) switch_check(i);
  return (switches_debounced & (1<<i)) ? true : false;
}


bool host_read_function_switch_edge(byte i)
{
  if( function_switch_irq[i]==0 ) switch_check(i);
  uint16_t bitval = 1<<i;
  bool b = switches_pulse & bitval ? true : false;
  if( b ) switches_pulse &= ~bitval;
  return b;
}


uint16_t host_read_function_switches_edge()
{
  for(byte i=0; i<16; i++) 
    if( function_switch_irq[i]==0 ) 
      switch_check(i);

  uint16_t res = switches_pulse;
  switches_pulse &= ~res;
  return res;
}


static void switch_interrupt(int i)
{
  switch_check(i);
}  


static void switch_interrupt1() { switch_interrupt(SW_STOP);     }
static void switch_interrupt2() { switch_interrupt(SW_RESET);    }
static void switch_interrupt3() { switch_interrupt(SW_CLR);      }
static void switch_interrupt4() { switch_interrupt(SW_AUX2UP);   }
static void switch_interrupt5() { switch_interrupt(SW_AUX2DOWN); }


static void switches_setup()
{
  attachInterrupt(digitalPinToInterrupt(function_switch_pin[SW_STOP]),     switch_interrupt1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(function_switch_pin[SW_RESET]),    switch_interrupt2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(function_switch_pin[SW_CLR]),      switch_interrupt3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(function_switch_pin[SW_AUX2UP]),   switch_interrupt4, CHANGE);
  attachInterrupt(digitalPinToInterrupt(function_switch_pin[SW_AUX2DOWN]), switch_interrupt5, CHANGE);

  delay(1);
  for(byte i=0; i<16; i++) debounceTime[i]=0;
  switches_debounced = 0;
  switches_pulse     = 0;
}


// --------------------------------------------------------------------------------------------------

void host_setup()
{
  int i;
  for(i=0; i<8; i++)
    {
      pinMode(2+i,  INPUT_PULLUP);  
      pinMode(14+i, INPUT_PULLUP);
    }
  for(i=10; i<14; i++) pinMode(i, OUTPUT);
  for(i=22; i<54; i++) pinMode(i, OUTPUT);

  switches_setup();

  // TODO: Find a way to initialize random number generator. Unfortunately 
  //       this is hard since all analog pins are connected and therefore the 
  //       usual analogRead(0) method always returns either 0 or 1023, depending
  //       on the setting of SW0
  randomSeed(analogRead(0));
}

#endif

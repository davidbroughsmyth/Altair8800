#ifdef __SAM3X8E__

#include <Arduino.h>
#include <DueFlashStorage.h>
#include "Altair8800.h"
#include "config.h"
#include "host_due.h"
#include "mem.h"
#include "cpucore.h"

#include <SPI.h>
#include <SD.h>

/*
  NOTE:
  Change -Os to -O3 (to switch optimization from size to performance) in:
  c:\Users\[user]\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.9\platform.txt

  ---- front panel connections by function:

  For pins that are not labeled on the board with their digital number
  the board label is given in []

  Function switches:
     RUN          => D20 (PIOB12)
     STOP         => D21 (PIOB13)
     STEP         => D54 [A0] (PIOA16)
     SLOW         => D55 [A1] (PIOA24)
     EXAMINE      => D56 [A2] (PIOA23)
     EXAMINE NEXT => D57 [A3] (PIOA22)
     DEPOSIT      => D58 [A4] (PIOA6)
     DEPOSIT NEXT => D59 [A5] (PIOA4)
     RESET        => D52 (PIOB21)
     CLR          => D53 (PIOB14)
     PROTECT      => D60 [A6] (PIOA3)
     UNPROTECT    => D61 [A7] (PIOA2)
     AUX1 UP      => D30 (PIOD9)
     AUX1 DOWN    => D31 (PIOA7)
     AUX2 UP      => D32 (PIOD10)
     AUX2 DOWN    => D33 (PIOC1)

   Address switches:
     SW0...7      => D62 [A8], D63 [A9], D64 [A10], D65 [A11], D66 [DAC0], D67 [DAC1], D68 [CANRX], D69 [CANTX]
     SW8...15     => D17,D16,D23,D24,D70[SDA1],D71[SCL1],D42,D43  (PIOA, bits 12-15,17-20)

   Bus LEDs:
     A0..7        => 34, 35, ..., 41          (PIOC, bits 2-9)
     A8..15       => 51, 50, ..., 44          (PIOC, bits 12-19)
     D0..8        => 25,26,27,28,14,15,29,11  (PIOD, bits 0-7)

   Status LEDs:
     INT          => D2  (PIOB25)
     WO           => D3  (PIOC28)
     STACK        => D4  (PIOC26)
     HLTA         => D5  (PIOC25)
     OUT          => D6  (PIOC24)
     M1           => D7  (PIOC23)
     INP          => D8  (PIOC22)
     MEMR         => D9  (PIOC21)
     INTE         => D12 (PIOD8)
     PROT         => D13 (PIOB27)
     WAIT         => D10 (PIOC29)
     HLDA         => D22 (PIOB26)


  ---- front panel connections by Arduino pin:

    D0  => Serial0 RX (in)
    D1  => Serial0 TX (out)
    D2  => INT        (out)
    D3  => WO         (out)
    D4  => STACK      (out)
    D5  => HLTA       (out)
    D6  => OUT        (out)
    D7  => M1         (out)
    
    D8  => INP        (out)
    D9  => MEMR       (out)
    D10 => WAIT       (out)
    D11 => D7         (out)
    D12 => INTE       (out)
    D13 => PROT       (out)

    D14 => D4         (out)
    D15 => D5         (out)
    D16 => SW9        (in)
    D17 => SW8        (in)
    D18 => Serial1 TX (out)
    D19 => Serial1 RX (in)
    D20 => RUN        (in)
    D21 => STOP       (in)
    
    D22 => HLDA       (out)
    D23 => SW10       (in)
    D24 => SW11       (in)
    D25 => D0         (out)
    D26 => D1         (out)
    D27 => D2         (out)
    D28 => D3         (out)
    D29 => D6         (out)
    D30 => AUX1 UP    (in)
    D31 => AUX1 DOWN  (in)
    D32 => AUX2 UP    (in)
    D33 => AUX2 DOWN  (in)
    D34 => A0         (out)
    D35 => A1         (out)
    D36 => A2         (out)
    D37 => A3         (out)
    D38 => A4         (out)
    D39 => A5         (out)
    D40 => A6         (out)
    D41 => A7         (out)
    D42 => SW14       (in)
    D43 => SW15       (in)
    D44 => A15        (out)
    D45 => A14        (out)
    D46 => A13        (out)
    D47 => A12        (out)
    D48 => A11        (out)
    D49 => A10        (out)
    D50 => A9         (out)
    D51 => A8         (out)
    D52 => RESET      (in)
    D53 => CLR        (in)

    The following are not labeled as digital pins on the board
    (i.e. not labeled Dxx) but can be used as digital pins.
    The board label for the pins is shown in parentheses.

    D54 (A0)    => STEP         (in)
    D55 (A1)    => SLOW         (in)
    D56 (A2)    => EXAMINE      (in)
    D57 (A3)    => EXAMINE NEXT (in)
    D58 (A4)    => DEPOSIT      (in)
    D59 (A5)    => DEPOSIT NEXT (in)
    D60 (A6)    => PROTECT      (in)
    D61 (A7)    => UNPROTECT    (in)
    D62 (A8)    => SW0          (in)
    D63 (A9)    => SW1          (in)
    D64 (A10)   => SW2          (in)
    D65 (A11)   => SW3          (in)
    D66 (DAC0)  => SW4          (in)
    D67 (DAC1)  => SW5          (in)
    D68 (CANRX) => SW6          (in)
    D69 (CANTX) => SW7          (in)

    D70 (SDA1)  => SW12         (in)
    D71 (SCL1)  => SW13         (in)

  ---- front panel connections by Processor register:
 
  PIOA:
    0 => SW7          (in)
    1 => SW6          (in)
    2 => UNPROTECT    (in)     
    3 => PROTECT      (in)     
    4 => DEPOSIT NEXT (in)     
    6 => DEPOSIT      (in)     
    7 => AUX1 DOWN    (in)
   12 => SW8          (in)     
   13 => SW9          (in)     
   14 => SW10         (in)     
   15 => SW11         (in)
   16 => STEP         (in)     
   17 => SW12         (in)
   18 => SW13         (in)
   19 => SW14         (in)
   20 => SW15         (in)
   22 => EXAMINE NEXT (in)     
   23 => EXAMINE      (in)     
   24 => SLOW         (in)     

  PIOB:
   12 => RUN       (in)
   13 => STOP      (in)
   14 => CLR       (in)
   15 => SW4       (in)
   16 => SW5       (in)
   17 => SW0       (in)
   18 => SW1       (in)
   19 => SW2       (in)
   20 => SW3       (in)
   21 => RESET     (in)
   25 => INT       (out)
   26 => HLDA      (out)
   17 => PROT      (out)

  PIOC:
    1 => AUX2 DOWN (in)
    2 => A0        (out)
    3 => A1        (out)
    4 => A2        (out)
    5 => A3        (out)
    6 => A4        (out)
    7 => A5        (out)
    8 => A6        (out)
    9 => A7        (out)
   12 => A8        (out)
   13 => A9        (out)
   14 => A10       (out)
   15 => A11       (out)
   16 => A12       (out)
   17 => A13       (out)
   18 => A14       (out)
   19 => A15       (out)
   21 => MEMR      (out)
   22 => INP       (out)
   23 => M1        (out)
   24 => OUT       (out)
   25 => HLTA      (out)
   26 => STACK     (out)
   28 => WO        (out)
   29 => WAIT      (out)

  PIOD:
    0 => D0        (out)
    1 => D1        (out)
    2 => D2        (out)
    3 => D3        (out)
    4 => D4        (out)
    5 => D5        (out)
    6 => D6        (out)
    7 => D7        (out)
    8 => INTE      (out)
    9 => AUX1 UP   (in)
   10 => AUX2 UP   (in)
*/


#define GETBIT(reg, regbit, v) (REG_PIO ## reg ## _PDSR & (1<<(regbit)) ? v : 0)
#define SETBIT(v, vbit, reg, regbit) if( v & vbit ) REG_PIO ## reg ## _SODR = 1<<regbit; else REG_PIO ## reg ## _CODR = 1<<regbit


uint16_t host_read_status_leds()
{
  uint16_t res = 0;
  res |= GETBIT(B, 25, ST_INT);
  res |= GETBIT(C, 28, ST_WO);
  res |= GETBIT(C, 26, ST_STACK);
  res |= GETBIT(C, 25, ST_HLTA);
  res |= GETBIT(C, 24, ST_OUT);
  res |= GETBIT(C, 23, ST_M1);
  res |= GETBIT(C, 22, ST_INP);
  res |= GETBIT(C, 21, ST_MEMR);
  res |= GETBIT(D,  8, ST_INTE);
  res |= GETBIT(B, 27, ST_PROT);
  res |= GETBIT(C, 10, ST_WAIT);
  res |= GETBIT(B, 26, ST_HLDA);
  return res;
}


byte host_read_data_leds()
{
  // D0..8 => PIOD, bits 0-7
  return REG_PIOD_PDSR & 0xff;
}


uint16_t host_read_addr_leds()
{
  // A0..7  => PIOC, bits 2-9
  // A8..15 => PIOC, bits 12-19
  word w = REG_PIOC_PDSR;
  return ((w & 0x000ff000) >> 4) | ((w & 0x000003fc) >> 2);
}


//------------------------------------------------------------------------------------------------------


uint16_t host_read_addr_switches()
{
  uint16_t v = 0;
  if( !digitalRead(62) ) v |= 0x01;
  if( !digitalRead(63) ) v |= 0x02;
  if( !digitalRead(64) ) v |= 0x04;
  if( !digitalRead(65) ) v |= 0x08;
  if( !digitalRead(66) ) v |= 0x10;
  if( !digitalRead(67) ) v |= 0x20;
  if( !digitalRead(68) ) v |= 0x40;
  if( !digitalRead(69) ) v |= 0x80;
  return v | (host_read_sense_switches() * 256);
}


//------------------------------------------------------------------------------------------------------


volatile static bool serial_timer_running = false;

void TC7_Handler()
{
  TC_GetStatus(TC2, 1);
  if( Serial.available() )
    altair_receive_serial_data(Serial.read());
  else 
    { TC_Stop(TC2, 1); serial_timer_running = false; }
}


void serial_interrupt()
{
  if( !serial_timer_running )
    { serial_timer_running = true; TC_Start(TC2, 1); }
}


// period_us is the timer period in microseconds (i.e. the time span after the timer goes off) 
void timer_setup(uint32_t period_us)
{
  // turn on the timer clock in the power management controller
  pmc_set_writeprotect(false);     // disable write protection for pmc registers
  pmc_enable_periph_clk(ID_TC7);   // enable peripheral clock TC7

  // we want wavesel 01 with RC
  TC_Configure(TC2,1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK3); 

  // enable timer interrupts on the timer
  TC2->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;   // IER = interrupt enable register
  TC2->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;  // IDR = interrupt disable register

  // Enable the interrupt in the nested vector interrupt controller
  // TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number (=(1*3)+1) for timer1 channel1
  NVIC_EnableIRQ(TC7_IRQn);

  // set the timer period. CLOCK3 is 2.65 MHz so if we set the
  // timer to 2625000/period_us then timer will go off after period_us microseconds
  TC_SetRC(TC2, 1, 2625000 / period_us);
  serial_timer_running = false;
}


//------------------------------------------------------------------------------------------------------

static bool use_sd = false;
uint32_t due_storagesize = 0xC000;


// The Due has 512k FLASH memory (addresses 0x00000-0x7ffff).
// We use 48k (0xC000 bytes) for storage
// DueFlashStorage address 0 is the first address of the second memory bank,
// i.e. 0x40000. We add 0x3C000 so we use at 0x74000-0x7ffff
// => MUST make sure that our total program size (shown in Arduine IDE after compiling)
//    is less than 475135 (0x73fff)! Otherwise we would overwrite our own program when
//    saving memory pages.
#define FLASH_STORAGE_OFFSET 0x34000
DueFlashStorage dueFlashStorage;

#define MOVE_BUFFER_SIZE 1024
byte moveBuffer[MOVE_BUFFER_SIZE];


static bool host_seek_file_write(File f, uint32_t addr)
{
  f.seek(addr);
  if( f.position()<addr )
    {
      memset(moveBuffer, 0, MOVE_BUFFER_SIZE);
      while( f.size()+MOVE_BUFFER_SIZE <= addr )
        f.write(moveBuffer, MOVE_BUFFER_SIZE);
      if( f.size() < addr )
        f.write(moveBuffer, addr-f.size());
    }

  return f.position()==addr;
}


static void host_write_data_flash(const void *data, uint32_t addr, uint32_t len)
{
  uint32_t offset = addr & 3;
  if( offset != 0)
    {
      byte buf[4];
      uint32_t alignedAddr = addr & 0xfffffffc;
      memcpy(buf, dueFlashStorage.readAddress(FLASH_STORAGE_OFFSET + alignedAddr), 4);
      memcpy(buf+offset, data, min(4-offset, len));
      dueFlashStorage.write(FLASH_STORAGE_OFFSET + alignedAddr, buf, 4);
      if( offset + len > 4 )
        dueFlashStorage.write(FLASH_STORAGE_OFFSET + alignedAddr + 4, ((byte *) data) + (4-offset), len - (4-offset));
    }
  else
    dueFlashStorage.write(FLASH_STORAGE_OFFSET + addr, (byte *) data, len);
}


static void host_read_data_flash(void *data, uint32_t addr, uint32_t len)
{
  memcpy(data, dueFlashStorage.readAddress(FLASH_STORAGE_OFFSET + addr), len);
}


static File storagefile;

static bool host_init_data_sd(const char *filename)
{
  storagefile = SD.open(filename, FILE_WRITE);
  return storagefile ? true : false;
}


static void host_write_data_sd(const void *data, uint32_t addr, uint32_t len)
{
  if( storagefile )
    {
      bool hlda = (host_read_status_leds() & ST_HLDA)!=0;
      if( host_seek_file_write(storagefile, addr) )
        {
          storagefile.write((byte *) data, len);
          storagefile.flush();
        }
      if( hlda ) host_set_status_led_HLDA(); else host_clr_status_led_HLDA();
    }
}


static void host_read_data_sd(void *data, uint32_t addr, uint32_t len)
{
  if( storagefile )
    {
      bool hlda = (host_read_status_leds() & ST_HLDA)!=0;
      if( storagefile.seek(addr) )
        {
          storagefile.read((byte *) data, len);
          storagefile.flush();
        }
      if( hlda ) host_set_status_led_HLDA(); else host_clr_status_led_HLDA();
    }
}


void host_write_data(const void *data, uint32_t addr, uint32_t len)
{
  if( use_sd )
    host_write_data_sd(data, addr, len);
  else
    host_write_data_flash(data, addr, len);
}

void host_read_data(void *data, uint32_t addr, uint32_t len)
{
  if( use_sd )
    host_read_data_sd(data, addr, len);
  else
    host_read_data_flash(data, addr, len);
}


void host_move_data(uint32_t to, uint32_t from, uint32_t len)
{
  uint32_t i;
  if( from < to )
    {
      for(i=0; i+MOVE_BUFFER_SIZE<len; i+=MOVE_BUFFER_SIZE)
        {
          host_read_data(moveBuffer, from+len-i-MOVE_BUFFER_SIZE, MOVE_BUFFER_SIZE);
          host_write_data(moveBuffer, to+len-i-MOVE_BUFFER_SIZE, MOVE_BUFFER_SIZE);
        }

      if( i<len )
        {
          host_read_data(moveBuffer, from, len-i);
          host_write_data(moveBuffer, to, len-i);
        }
    }
  else
    {
      for(i=0; i+MOVE_BUFFER_SIZE<len; i+=MOVE_BUFFER_SIZE)
        {
          host_read_data(moveBuffer, from+i, MOVE_BUFFER_SIZE);
          host_write_data(moveBuffer, to+i, MOVE_BUFFER_SIZE);
        }

      if( i<len )
        {
          host_read_data(moveBuffer, from+i, len-i);
          host_write_data(moveBuffer, to+i, len-i);
        }
    }
}

void host_copy_flash_to_ram(void *dst, const void *src, uint32_t len)
{
  memcpy(dst, src, len);
}


// --------------------------------------------------------------------------------------------------


volatile static uint16_t switches_pulse = 0;
volatile static uint16_t switches_debounced = 0;
static uint32_t debounceTime[16];
static const byte function_switch_pin[16] = {20,21,54,55,56,57,58,59,52,53,60,61,30,31,32,33};
static const uint16_t function_switch_irq[16] = {0, INT_SW_STOP, 0, 0, 0, 0, 0, 0, INT_SW_RESET, INT_SW_CLR, 
                                                 0, 0, 0, 0, INT_SW_AUX2UP, INT_SW_AUX2DOWN};


bool host_read_function_switch(byte i)
{
  return !digitalRead(function_switch_pin[i]);
}


bool host_read_function_switch_debounced(byte i)
{
  return (switches_debounced & (1<<i)) ? true : false;
}


bool host_read_function_switch_edge(byte i)
{
  uint16_t bitval = 1<<i;
  bool b = switches_pulse & bitval ? true : false;
  if( b ) switches_pulse &= ~bitval;
  return b;
}


uint16_t host_read_function_switches_edge()
{
  uint16_t res = switches_pulse;
  switches_pulse &= ~res;
  return res;
}


static void switch_interrupt(int i)
{
  if( millis()>debounceTime[i] )
    {
      uint16_t bitval = 1<<i;

      bool d1 = !digitalRead(function_switch_pin[i]);
      bool d2 = (switches_debounced & bitval) ? true : false;

      if( d1 && !d2 ) 
        {
          switches_debounced |= bitval;
          switches_pulse |= bitval;
          if( function_switch_irq[i]>0 ) altair_interrupt(function_switch_irq[i]);
          debounceTime[i] = millis() + 100;
        }
      else if( !d1 && d2 ) 
        {
          switches_debounced &= ~bitval;
          switches_pulse &= ~bitval;
          debounceTime[i] = millis() + 100;
        }
    }
}


static void switch_interrupt_0()  { switch_interrupt(0);  }
static void switch_interrupt_1()  { switch_interrupt(1);  }
static void switch_interrupt_2()  { switch_interrupt(2);  }
static void switch_interrupt_3()  { switch_interrupt(3);  }
static void switch_interrupt_4()  { switch_interrupt(4);  }
static void switch_interrupt_5()  { switch_interrupt(5);  }
static void switch_interrupt_6()  { switch_interrupt(6);  }
static void switch_interrupt_7()  { switch_interrupt(7);  }
static void switch_interrupt_8()  { switch_interrupt(8);  }
static void switch_interrupt_9()  { switch_interrupt(9);  }
static void switch_interrupt_10() { switch_interrupt(10); }
static void switch_interrupt_11() { switch_interrupt(11); }
static void switch_interrupt_12() { switch_interrupt(12); }
static void switch_interrupt_13() { switch_interrupt(13); }
static void switch_interrupt_14() { switch_interrupt(14); }
static void switch_interrupt_15() { switch_interrupt(15); }


static void switches_setup()
{
  attachInterrupt(function_switch_pin[ 0], switch_interrupt_0,  CHANGE);
  attachInterrupt(function_switch_pin[ 1], switch_interrupt_1,  CHANGE);
  attachInterrupt(function_switch_pin[ 2], switch_interrupt_2,  CHANGE);
  attachInterrupt(function_switch_pin[ 3], switch_interrupt_3,  CHANGE);
  attachInterrupt(function_switch_pin[ 4], switch_interrupt_4,  CHANGE);
  attachInterrupt(function_switch_pin[ 5], switch_interrupt_5,  CHANGE);
  attachInterrupt(function_switch_pin[ 6], switch_interrupt_6,  CHANGE);
  attachInterrupt(function_switch_pin[ 7], switch_interrupt_7,  CHANGE);
  attachInterrupt(function_switch_pin[ 8], switch_interrupt_8,  CHANGE);
  attachInterrupt(function_switch_pin[ 9], switch_interrupt_9,  CHANGE);
  attachInterrupt(function_switch_pin[10], switch_interrupt_10, CHANGE);
  attachInterrupt(function_switch_pin[11], switch_interrupt_11, CHANGE);
  attachInterrupt(function_switch_pin[12], switch_interrupt_12, CHANGE);
  attachInterrupt(function_switch_pin[13], switch_interrupt_13, CHANGE);
  attachInterrupt(function_switch_pin[14], switch_interrupt_14, CHANGE);
  attachInterrupt(function_switch_pin[15], switch_interrupt_15, CHANGE);

  delay(1);
  for(int i=0; i<16; i++) debounceTime[i]=0;
  switches_debounced = 0;
  switches_pulse     = 0;
}


// --------------------------------------------------------


signed char isinput[] = 
  {
   -1, // D0  => Serial0 RX (don't set)
   -1, // D1  => Serial0 TX (don't set)
    0, // D2  => INT
    0, // D3  => WO
    0, // D4  => STACK
    0, // D5  => HLTA
    0, // D6  => OUT
    0, // D7  => M1
    0, // D8  => INP
    0, // D9  => MEMR
    0, // D10 => WAIT
    0, // D11 => D7
    0, // D12 => INTE
    0, // D13 => PROT
    0, // D14 => D4
    0, // D15 => D5
    1, // D16 => SW9
    1, // D17 => SW8
   -1, // D18 => Serial1 TX (don't set)
   -1, // D19 => Serial1 RX (don't set)
    1, // D20 => RUN
    1, // D21 => STOP
    0, // D22 => HLDA
    1, // D23 => SW10
    1, // D24 => SW11
    0, // D25 => D0
    0, // D26 => D1
    0, // D27 => D2
    0, // D28 => D3
    0, // D29 => D6
    1, // D30 => AUX1 UP
    1, // D31 => AUX1 DOWN
    1, // D32 => AUX2 UP
    1, // D33 => AUX2 DOWN
    0, // D34 => A0
    0, // D35 => A1
    0, // D36 => A2
    0, // D37 => A3
    0, // D38 => A4
    0, // D39 => A5
    0, // D40 => A6
    0, // D41 => A7
    1, // D42 => SW14
    1, // D43 => SW15
    0, // D44 => A15
    0, // D45 => A14
    0, // D46 => A13
    0, // D47 => A12
    0, // D48 => A11
    0, // D49 => A10
    0, // D50 => A9
    0, // D51 => A8
    1, // D52 => RESET
    1, // D53 => CLR
    1, // D54 (A0)    => STEP
    1, // D55 (A1)    => SLOW
    1, // D56 (A2)    => EXAMINE
    1, // D57 (A3)    => EXAMINE NEXT
    1, // D58 (A4)    => DEPOSIT
    1, // D59 (A5)    => DEPOSIT NEXT
    1, // D60 (A6)    => PROTECT
    1, // D61 (A7)    => UNPROTECT
    1, // D62 (A8)    => SW0
    1, // D63 (A9)    => SW1
    1, // D64 (A10)   => SW2
    1, // D65 (A11)   => SW3
    1, // D66 (DAC0)  => SW4
    1, // D67 (DAC1)  => SW5
    1, // D68 (CANRX) => SW6
    1, // D69 (CANTX) => SW7
    1, // D70 (SCL1)  => SW13
    1, // D71 (SDA1)  => SW12
  };


uint32_t host_get_random()
{
  delayMicroseconds(1);
  return (uint32_t) trng_read_output_data(TRNG);
}


void host_setup()
{
  // define digital input/output pin direction
  for(int i=0; i<72; i++)
    if( isinput[i]>=0 )
      pinMode(i, isinput[i] ? INPUT_PULLUP : OUTPUT);

  // enable pull-up resistor on RX1, otherwise some serial devices
  // (e.g. serial-to-bluetooth) won't work
  pinMode(19, INPUT_PULLUP);
  
  // attach interrupts
  switches_setup();

  // at 9600 baud with 1 stop bit it takes 1/9600*(8+1)=0.0009375 seconds to
  // receive one character, so a byte should have been received 1ms (1000 microseconds)
  // after we get an interrupt signaling activity on the serial line
  timer_setup(1000);

  // interrupt to see activity on serial RX pin
  attachInterrupt(digitalPinToInterrupt(19), serial_interrupt, RISING);

  // set mask for bits that will be written to via REG_PIOX_OSDR
  REG_PIOC_OWDR = 0xFFF00C03;  // address bus (16 bit)
  REG_PIOD_OWDR = 0xFFFFFF00;  // data bus (8 bit)

  // init random number generator
  pmc_enable_periph_clk(ID_TRNG);
  trng_enable(TRNG);
  trng_read_output_data(TRNG);

  // check if SD card available (send "chip select" signal to HLDA status light)
  bool hlda = (host_read_status_leds() & ST_HLDA)!=0;
  if( SD.begin(22) && host_init_data_sd("STORAGE.DAT") )
    {
      use_sd = true;
      due_storagesize = 512*1024;
    }
  else
    due_storagesize = 0xFFFF;

  // restore HLDA status light to what it was before
  if( hlda ) host_set_status_led_HLDA(); else host_clr_status_led_HLDA();
}


uint32_t host_read_file(const char *filename, uint32_t offset, uint32_t len, void *buffer)
{
  uint32_t res = 0;

  if( use_sd )
    {
      bool hlda = (host_read_status_leds() & ST_HLDA)!=0;
      File f = SD.open(filename, FILE_READ);
      if( f )
        {
          if( f.seek(offset) && f.position()==offset )
            res = f.read((uint8_t *) buffer, len);
          f.close();
        }
      if( hlda ) host_set_status_led_HLDA(); else host_clr_status_led_HLDA();
    }

  return res;
}


uint32_t host_write_file(const char *filename, uint32_t offset, uint32_t len, void *buffer)
{
  uint32_t res = 0;

  if( use_sd )
    {
      bool hlda = (host_read_status_leds() & ST_HLDA)!=0;
      File f = SD.open(filename, FILE_WRITE);
      if( f )
        {
          if( host_seek_file_write(f, offset) )
            res = f.write((uint8_t *) buffer, len);
          f.close();
        }
      if( hlda ) host_set_status_led_HLDA(); else host_clr_status_led_HLDA();
    }

  return res;
}


#endif

// filename  ***************  LCD.C ****************************** 
// LCD Display (HD44780) on Port AD,M for the 9S12C32   
// Jonathan W. Valvano 1/14/07

//  This example accompanies the books
//   "Embedded Microcomputer Systems: Real Time Interfacing",
//        Thomson Engineering, copyright (c) 2006,
//   "Introduction to Embedded Microcomputer Systems: 
//    Motorola 6811 and 6812 Simulation", Thomson, copyright (c) 2002

// Copyright 2007 by Jonathan W. Valvano, valvano@mail.utexas.edu 
//    You may use, edit, run or distribute this file 
//    as long as the above copyright notice remains 

/*   
  size is 1*16 
  if do not need to read busy, then you can tie R/W=ground 
  ground = pin 1    Vss
  power  = pin 2    Vdd   +5V
  ground = pin 3    Vlc   grounded for highest contrast
  PM5    = pin 4    RS    (1 for data, 0 for control/status)
  PAD0   = pin 5    R/W   (1 for read, 0 for write)
  PM4    = pin 6    E     (enable)
  PM3    = pin 14   DB7   (4-bit data)
  PM2    = pin 13   DB6
  PM1    = pin 12   DB5
  PM0    = pin 11   DB4
16 characters are configured as 2 rows of 8
addr  00 01 02 03 04 05 06 07 40 41 42 43 44 45 46 47
*/

#include <mc9s12c32.h>  /* derivative information */
#include "LCD.H"

static unsigned short OpenFlag=0;

//---------------------wait---------------------
// time delay
// Input: time in 0.667usec
// Output: none    ***************

void static wait(unsigned short delay){ 
unsigned short startTime;
  startTime = TCNT;
  while((TCNT-startTime) <= delay){}  
}
  
//---------------------outCsr---------------------
// sends one command code to the LCD control/status
// Input: command is 8-bit function to execute
// Output: none
static void outCsr(unsigned char command){
  //SCI_OutString("Send Command: ");OutCRLF();
  PTT = ((0x0F&(command>>4))+128); // ms nibble, E=0, RS=0   DONE
  //SCI_OutUDec(PTT);   OutCRLF();
  PTT |= 0x10;             // E goes 0,1 DONE
  //SCI_OutUDec(PTT);   OutCRLF();  
  PTT &= ~0x10;            // E goes 1,0 DONE
  //SCI_OutUDec(PTT);   OutCRLF();
  PTT = ((command&0x0F)+128);      // ls nibble, E=0, RS=0  DONE
  //SCI_OutUDec(PTT);   OutCRLF();
  PTT |= 0x10;             // E goes 0,1 DONE
  //SCI_OutUDec(PTT);   OutCRLF();
  PTT &= ~0x10;            // E goes 1,0  DONE
  //SCI_OutUDec(PTT);   OutCRLF();
  wait(135);               // blind cycle 90 us wait

}

//---------------------LCD_Clear---------------------
// clear the LCD display, send cursor to home
// Input: none
// Output: true if successful
short LCD_Clear(void){
  if(OpenFlag==0){
    return 0;  // not open
  }
  //SCI_OutString("LCD Clear: ");OutCRLF();
  outCsr(0x01);        // Clear Display
  wait(2460);          // 1.64ms wait
  outCsr(0x02);        // Cursor to home
  wait(2460);          // 1.64ms wait
  return 1;		         // success
}

//---------------------LCD_Open---------------------
// initialize the LCD display, called once at beginning
// Input: display determines increment and shift option
//        cursor determines cursor options
//        move determines cursor movement
//        size sets display size
// Output: true if successful

short LCD_Open(char display, char cursor, char move, char size){
int i;
  if(OpenFlag){
    return 0;      // error if already open
  }
  DDRT |= 0xFF;   // Port T 0-6 bits for output DONE
  
  SCI_OutString("LCDOpen DDRT: ");SCI_OutUDec(DDRT);OutCRLF();
  //ATDDIEN |= 0x01; // enable AD bit 0 digital driver NOT NEEDED 
  PTT &= 0xBF;   // PTT6=R/W=0 means write
  SCI_OutString("LCDOpen PTT: ");SCI_OutUDec(PTT);OutCRLF();
  //SCI_InUDec(); 
  //DDRM = 0x3F;     // PM5-0 output to LCD NOT NEEDED
  //TSCR1 = 0x80;   // Enable TCNT, 8MHz in run modes
  //TSCR2 = 0x03;   // divide by 8 TCNT prescale, TCNT at 1usec
  //PACTL = 0;      // timer prescale used for TCNT
/* Bottom three bits of TSCR2 (PR2,PR1,PR0) determine TCNT period
    divide  at 24MHz    
000   1     42ns  TOF  2.73ms  
001   2     84ns  TOF  5.46ms  
010   4    167ns  TOF  10.9ms      
011   8    333ns  TOF  21.8ms 	 
100  16    667ns  TOF  43.7ms 	 
101  32   1.33us  TOF  87.4ms		 
110  64   2.67us  TOF 174.8ms   
111 128   5.33us  TOF 349.5ms    */ 
// Be careful, TSCR1 and TSCR2 maybe set in other rituals
  
  for(i=100;i;i--){
    wait(1500);    // 100ms to allow LCD powerup
  }
  

  PTT = 0x82;		 // DL=0 4-bit, RS=0 (command), E=0  DONE
  //SCI_InUDec();
  SCI_OutString("LCDOpen PTT2 (0x82): ");
  //SCI_InUDec();
  PTT |= 0x10;   // E goes 0,1 DONE
  SCI_OutString("LCDOpen PTT3 (E high) : ");
  //SCI_InUDec();
  PTT &= ~0x10;  // E goes 1,0 DONE
  SCI_OutString("LCDOpen PTT4 (E low) : ");
  //SCI_InUDec();
  wait(135);     // 90 us wait


  outCsr(0x04+display); // I/D Increment, S displayshift
  outCsr(0x0C+cursor);  // D=1 displayon, C cursoron/off, B blink on/off
  outCsr(0x14+move);    // S/C cursormove, R/L shiftright
  outCsr(0x20+size);    // DL=0 4bit, N 1/2 line, F 5by7 or 5by10
  OpenFlag = 1;         // device open
  return LCD_Clear();   // clear display
}

//---------------------LCD_OutChar---------------------
// sends one ASCII to the LCD display
// Input: letter is ASCII code
// Output: true if successful
short LCD_OutChar(unsigned char letter){
  if(OpenFlag==0){
    return 0;  // not open
  }
  PTT = 0xA0+(0x0F&(letter>>4));        // ms nibble, E=0, RS=0 DONE
  
  PTT |= 0x10;       // E goes 0,1 DONE
  PTT &= ~0x10;      // E goes 1,0 DONE
  
  PTT = 0xA0+(letter&0x0F);        // ls nibble, E=0, RS=0  DONE
  PTT |= 0x10;       // E goes 0,1 DONE
  PTT &= ~0x10;      // E goes 1,0 DONE
  wait(135);         // 90 us wait
  return 1;	         // success
}

//---------------------LCD_OutString--------------
// Display String
// Input: pointer to NULL-terminationed ASCII string 
// Output: true if successful
short LCD_OutString(char *pt){ 
  if(OpenFlag==0){
    return 0;  // not open
  }
  while(*pt){
    if(LCD_OutChar((unsigned char)*pt)==0){
      return 0;
    }
    pt++;
  }
  return 1;	  // success
}





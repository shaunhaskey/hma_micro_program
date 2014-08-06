// filename  ***************  SCI.C ****************************** 
// Simple I/O routines to 9S12C32 serial port   
// Jonathan W. Valvano 2/14/06

//  This example accompanies the books
//   "Embedded Microcomputer Systems: Real Time Interfacing",
//        Thompson, copyright (c) 2006,
//   "Introduction to Embedded Microcomputer Systems: 
//    Motorola 6811 and 6812 Simulation", Brooks-Cole, copyright (c) 2002

// Copyright 2006 by Jonathan W. Valvano, valvano@mail.utexas.edu 
//    You may use, edit, run or distribute this file 
//    as long as the above copyright notice remains 
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan + Mingjie Qiu
 
#include <mc9s12c32.h>     /* derivative information */
#include "SCI.h"
#define RDRF 0x20   // Receive Data Register Full Bit
#define TDRE 0x80   // Transmit Data Register Empty Bit

//-------------------------SCI_Init------------------------
// Initialize Serial port SCI
// Input: baudRate is tha baud rate in bits/sec
// Output: none
// SCIBDL=250000/baudRate, these 5 cases only
// baudRate =  2400 bits/sec   SCIBDL=104
// baudRate =  4800 bits/sec   SCIBDL=52
// baudRate =  9600 bits/sec   SCIBDL=26
// baudRate =  19200 bits/sec  SCIBDL=13
// baudRate =  250000 bits/sec SCIBDL=1
// assumes a module clock frequency of 4 MHz
// sets baudRate to 250000 bits/sec if doesn't match one of the above
void SCI_Init(unsigned short baudRate){
  SCIBDH = 0;   // br=MCLK/(16*baudRate) 
  
  switch(baudRate){
    case 2400:  SCIBDL=104; break;
    case 4800:  SCIBDL=52;  break;
    case 9600:  SCIBDL=26;  break;
    case 19200: SCIBDL=13;  break;
    default:    SCIBDL = 1 ;  // 250000 
  }
  SCICR1 = 0;
/* bit value meaning
    7   0    LOOPS, no looping, normal
    6   0    WOMS, normal high/low outputs
    5   0    RSRC, not appliable with LOOPS=0
    4   0    M, 1 start, 8 data, 1 stop
    3   0    WAKE, wake by idle (not applicable)
    2   0    ILT, short idle time (not applicable)
    1   0    PE, no parity
    0   0    PT, parity type (not applicable with PE=0) */ 
  SCICR2 = 0x0C; 
/* bit value meaning
    7   0    TIE, no transmit interrupts on TDRE
    6   0    TCIE, no transmit interrupts on TC
    5   0    RIE, no receive interrupts on RDRF
    4   0    ILIE, no interrupts on idle
    3   1    TE, enable transmitter
    2   1    RE, enable receiver
    1   0    RWU, no receiver wakeup
    0   0    SBK, no send break */ 
}
    
//-------------------------SCI_InChar------------------------
// Wait for new serial port input, busy-waiting synchronization
// The input is not echoed
// Input: none
// Output: ASCII code for key typed
char SCI_InChar(void){
  while((SCISR1 & RDRF) == 0){};
  return(SCIDRL);
}
        
//-------------------------SCI_OutChar------------------------
// Wait for buffer to be empty, output 8-bit to serial port
// busy-waiting synchronization
// Input: 8-bit data to be transferred
// Output: none
void SCI_OutChar(char data){
  while((SCISR1 & TDRE) == 0){};
  SCIDRL = data;
}

   
//-------------------------SCI_InStatus--------------------------
// Checks if new input is ready, TRUE if new input is ready
// Input: none
// Output: TRUE if a call to InChar will return right away with data
//         FALSE if a call to InChar will wait for input
char SCI_InStatus(void){
  return(SCISR1 & RDRF);
}

//-----------------------SCI_OutStatus----------------------------
// Checks if output data buffer is empty, TRUE if empty
// Input: none
// Output: TRUE if a call to OutChar will output and return right away
//         FALSE if a call to OutChar will wait for output to be ready
char SCI_OutStatus(void){
  return(SCISR1 & TDRE);
}


//-------------------------SCI_OutString------------------------
// Output String (NULL termination), busy-waiting synchronization
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void SCI_OutString(char *pt){
  while(*pt){
    SCI_OutChar(*pt);
    pt++;
  }
}

//----------------------SCI_InUDec-------------------------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 16 bit unsigned number
//     valid range is 0 to 65535
// Input: none
// Output: 16-bit unsigned number
// If you enter a number above 65535, it will truncate without an error
// Backspace will remove last digit typed


unsigned short SCI_InUDec(void){	
unsigned short number1=0, number2=0, number3=0, number4=0,number5=0, contnum=1, length=0;
char character;
  character = SCI_InChar();	
  while(character != CR){ // accepts until <enter> is typed

// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9') && (contnum==1)) {
      number1 = 10*number1+(character-'0');   // this line overflows if above 65535
      length++;
      SCI_OutChar(character);
    } 
 
     character = SCI_InChar();	
  }
  
  return number1;
  
}


//-----------------------SCI_OutUDec-----------------------
// Output a 16-bit number in unsigned decimal format
// Input: 16-bit number to be transferred
// Output: none
// Variable format 1-5 digits with no space before or after
void SCI_OutUDec(unsigned short n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string 
  if(n >= 10){
    SCI_OutUDec(n/10);
    n = n%10;
  }
  SCI_OutChar(n+'0'); /* n is between 0 and 9 */
}




//------------------------SCI_InString------------------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed 
//    or until max length of the string is reached.  
// It echoes each character as it is inputted.  
// If a backspace is inputted, the string is modified 
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void SCI_InString(char *bufPt, unsigned short max) {	
int length=0;
char character;
  character = SCI_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        SCI_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++; 
      SCI_OutChar(character);
    }
    character = SCI_InChar();
  }
  *bufPt = 0;
}

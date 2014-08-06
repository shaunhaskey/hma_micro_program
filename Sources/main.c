//Interesting Program

#include "derivative.h"      /* derivative-specific definitions */
#include <stdio.h>
#include <hidef.h>      /* common defines and macros */
#include <mc9s12c32.h>  /* derivative information */
#include "SCI.h"
#include "lcd.h"
#pragma LINK_INFO DERIVATIVE "mc9s12c32"

unsigned char OverallSettings[16][3][2]=0, OverallReceived[16][3][2]=0;
unsigned short TimeoutCount,InputTimeoutFlag=0,RTITrigger=0;
unsigned char Temp2IntShort,Temp1IntShort,Temp1Global,Temp2Global;
unsigned char globalidle=1;
//----------------------------------------------------------
//Function Definitions
void OutCRLF(void);
void Timer_Init(void);
void Timer_mwait(unsigned short);
void SPI_Init(void);
void SPI_Off();
void ADC_Init(void);
unsigned short ADC_In(unsigned short);
void check(short);
void Temperature(void);
char SCI_InChar2(void);
unsigned short SCI_InUDec2(void);
unsigned short transmitSPI(unsigned short);
unsigned short SCI_InUDec3(void);
char SCI_InCharInitial(void);
char SCI_InCharSecondary(void);

void PrintOutSettings(void);
void AmpSettingsInput(void);
void OutputWrite2(void);
void TransmitControl(void);
static void LCDCommand(unsigned char command);	
//---------------------OutCRLF---------------------
// Output a CR,LF to SCI to go to a new line
// Input: none
// Output: none
// toggle PortT bit 0 each time, debugging profile
void OutCRLF(void){
  SCI_OutChar(CR);
  SCI_OutChar(LF);
}



void Timer_Init(void){
  TSCR1 = 0x80;   // Enable TCNT, 8MHz in run modes
  TSCR2 = 0x02;   // divide by 8 TCNT prescale, TCNT at 1usec
  PACTL = 0;      // timer prescale used for TCNT
/* Bottom three bits of TSCR2 (PR2,PR1,PR0) determine TCNT period
    divide  8MHz E clock This is wrong, actual clock is 4MHz on the bus so divide/multiply by 2   
000   1    125ns    
001   2    250ns     
010   4    500ns         
011   8      1us   	 
100  16      2us   	 
101  32      4us  		 
110  64      8us     
111 128     16us      */ 
// Be careful, TSCR1 and TSCR2 maybe set in other rituals
}


void Timer_mwait(unsigned short msec){ 
unsigned short startTime;
  for(; msec>0; msec--){
    startTime = TCNT;
    while((TCNT-startTime) <= 1000){} 
  }
}


void Timer_Fast(unsigned short msec){ 
unsigned short startTime;
  
  TSCR1=0x0;//off to reset
  TSCR1=0x80;//on
  
  for(; msec>0; msec--){
    startTime = TCNT;
    while((TCNT-startTime) <= 1){} 
  }
}

//----------------------------------------------------------
void SPI_Init(void){
  //SPICR1=0x58; //0x56; // Setup the SPI Interface  
  //SPICR2=0x00;//0x10; //0x00;
  //SPIBR=0x67;
  /*
   0x64 ; 224 ; 17.86kHz
   0x65 ; 448 ; 8.93kHz
   0x66 ; 896 ; 4.46kHz
   0x67 ; 1792; 2.23kHz
    
  */


  
//  DDRM|=0x38;
//  DDRM &=~0x04;
//  PTM|=0x08; 
//  PERM &= ~0x04;            // E goes 1,0 DONE
   DDRM=0x3A; // Set direction of pins  
  PTM=0x00; //all pins low
  PERM &= ~0x04;//Disable pull up on MISO

}


//----------------------------------------------------------
void SPI_Off(){
  SPICR1=0x0;
}


//----------------------------------------------------------
//Initialise the ADC
void ADC_Init(void){
  ATDCTL2=0x80;
  ATDCTL3=0x08;
  ATDCTL4=0x05;//10bit //0x05 8 bit
}


//----------------------------------------------------------
//Obtain Data for the ADC
unsigned short ADC_In(unsigned short chan){
  ATDCTL5=(unsigned char)chan;//81;//87;
  while((ATDSTAT1&0x01)==0){};    // wait for CCF0 
  return ATDDR0; 
}


//----------------------------------------------------------
//Check to see if the LCD is working - output LCD Error if it isn't
//Should change this so that it doesn't go on forever.
void check(short status){	 // 0 if LCD is broken
  if(status ==0){		   
    for(;;) {
      SCI_OutString("LCDError");   OutCRLF();
      Timer_mwait(250);    // 0.25 sec wait
    }
  }
}



void Temperature(void){
  unsigned short ADCReceived, ADCReceived2, Res1IntShort,Res2IntShort;
  float Res1Float,Res2Float,Temp1Float,Temp2Float,ADCReceived1Float,ADCReceived2Float,Rref=2500;
  char LCDOutput[14];
  unsigned short LCDTemp1[3],LCDTemp2[3];
  unsigned char dummychar;

        
        ADCReceived=(ADC_In(0x87));//ADCReceived=(ADC_In(0x07)>>6); //  Read Analog value from AN7
        ADCReceived2=(ADC_In(0x86));//ADCReceived2=(ADC_In(0x06)>>6);//  Read Analog value from AN6
        
        ADCReceived2Float=(float) ADCReceived2;
        ADCReceived1Float=(float) ADCReceived;

        Res2Float= (ADCReceived2Float/1023/7.47*Rref);
        Res1Float= (ADCReceived1Float/1023/7.47*Rref);
        Res2IntShort=(unsigned short) Res2Float;
        Res1IntShort=(unsigned short) Res1Float;

        Temp2Float=  (Res2Float-100)/0.385;
        Temp1Float=  (Res1Float-100)/0.385;


        Temp2IntShort=(unsigned char) Temp2Float;
        Temp1IntShort=(unsigned char) Temp1Float;
        
        Temp1Global=Temp1IntShort;
        Temp2Global=Temp2IntShort;
        //SCI_OutString("Raw 1 :");SCI_OutUDec(ADCReceived);SCI_OutString(" Res1 :");SCI_OutUDec(Res1IntShort);SCI_OutString(" Temp1:");SCI_OutUDec(Temp1IntShort);SCI_OutString("  || Raw 2:");SCI_OutUDec(ADCReceived2);SCI_OutString(" Res : ");SCI_OutUDec(Res2IntShort);SCI_OutString(" Temp : ");SCI_OutUDec(Temp2IntShort);OutCRLF();
        
        LCDTemp2[0]= (Temp2IntShort/100);Temp2IntShort=Temp2IntShort-LCDTemp2[0]*100;
        LCDTemp2[1]= (Temp2IntShort/10);Temp2IntShort=Temp2IntShort-LCDTemp2[1]*10;
        LCDTemp2[2]= Temp2IntShort;
  
        LCDTemp1[0]= (Temp1IntShort/100);Temp1IntShort=Temp1IntShort-LCDTemp1[0]*100;
        LCDTemp1[1]= (Temp1IntShort/10);Temp1IntShort=Temp1IntShort-LCDTemp1[1]*10;
        LCDTemp1[2]= Temp1IntShort;
        

// Update the temperature values on the LCD
LCDCommand(0x80+0x07);//Goto first temperature address
        LCD_OutChar((LCDTemp1[0]+'0'));
        LCD_OutChar((LCDTemp1[1]+'0'));
        LCD_OutChar((LCDTemp1[2]+'0'));
              
LCDCommand(0x80+0x47); //Goto second temperature address
        LCD_OutChar((LCDTemp2[0]+'0'));
        LCD_OutChar((LCDTemp2[1]+'0'));
        LCD_OutChar((LCDTemp2[2]+'0'));

//Goto Idle Location        
        LCDCommand(0x80+0x4F);
        
//Change the character at this location so that we know the Micro is working        
        if(globalidle==1){
          LCD_OutChar(0xb0);
          globalidle=2;
        } else{
          LCD_OutChar(0x7c);
          globalidle=1;
        }
        
}


char SCI_InChar2(void){
  while((SCISR1 & 0x20) == 0){Temperature();};
  return(SCIDRL);
}



unsigned short SCI_InUDec2(void){	
unsigned short number1=0, number2=0, number3=0, number4=0,number5=0, contnum=1, length=0;
char character;
  character = SCI_InChar2();	
  while(character != CR){ // accepts until <enter> is typed

// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9') && (contnum==1)) {
      number1 = 10*number1+(character-'0');   // this line overflows if above 65535
      length++;
      SCI_OutChar(character);OutCRLF();
    } 
 
     character = SCI_InChar();	
  }
  
  return number1;
  
}




void interrupt 7 handler(){
  CRGFLG = 0x80;     // acknowledge, clear RTIF flag
  //RTITrigger=1;
  TimeoutCount++;           // number of interrupts
}




unsigned short SCI_InUDec3(void){
unsigned short number1=0, number2=0, number3=0, number4=0,number5=0, contnum=1, length=0;
char character;
  character = SCI_InCharInitial();	
  while((character != CR)&&(InputTimeoutFlag==0)){ // accepts until <enter> is typed or timeout activated

// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9') && (contnum==1)) {
      number1 = 10*number1+(character-'0');   // this line overflows if above 65535
      length++;
      SCI_OutChar(character);//OutCRLF();
    }
    character = SCI_InCharSecondary();	
  }

if(InputTimeoutFlag==1){
  return 66;
}

return number1;
}


char SCI_InCharInitial(void){
  CRGINT = 0x80; // RTIE=1 enable rti
  RTICTL = 0x7F; // Interrupt Every 0.131seconds
  TimeoutCount=0; //Reset Counter
  while(((SCISR1 & 0x20) == 0)){
    if(TimeoutCount==23){
      Temperature();TimeoutCount=0;
    }
  }
  
    CRGINT = 0x0; // RTIE=0 disable rti
    TimeoutCount=0; //Reset Counter 
    return(SCIDRL); //Return the input character
}



char SCI_InCharSecondary(void){
  CRGINT = 0x80; // RTIE=1 enable rti
  RTICTL = 0x7F; // Interrupt Every 0.131seconds
  TimeoutCount=0; //Reset Counter 
  //SCI_OutString("In Secondary");

  while(((SCISR1 & 0x20) == 0)&&(TimeoutCount<38)){};
  CRGINT = 0x0; // RTIE=0 disable rti
  TimeoutCount=0; //Reset Counter 

  if((SCISR1 & 0x20) == 0){
    InputTimeoutFlag=1; //Set Flag 
    return 0;
  }
  
  return(SCIDRL);
  }  


// Send a command to the LCD
static void LCDCommand(unsigned char command){
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
  Timer_mwait(135);               // blind cycle 90 us wait

}



//----------------------------------------------------------
void main(void) {		
  unsigned short MainSelection;
  unsigned int tempiterations;
  char character2;
  int  ii,jj, kk;
  
  asm cli
  
  DDRT |= 0x80; //For SYNC Line

  ADC_Init();
  SCI_Init(19200);	// fastest standard baud rate on run mode 9S12C32
  SPI_Init();
  PTT |= 0x80;    // SYNC High 
 
 
 
  Timer_Init();
  
  SCI_OutString("Starting LCD Open");   OutCRLF();
  
  check(LCD_Open(LCDINC+LCDNOSHIFT,
  LCDNOCURSOR+LCDBLINK,
  LCDNOSCROLL+LCDLEFT,
  LCD2LINE+LCD7DOT));
  
  SCI_OutString("1 - Finished LCD Open");   OutCRLF();
  
  LCD_Clear();  
  check(LCD_OutString("TorMirnArray"));
  
  
  Timer_mwait(1000);

LCD_Clear();

LCD_OutString("Temp1: ---");
LCD_OutChar(0xdf);
LCD_OutString("C");
LCDCommand(0x80+0x40);
LCD_OutString("Temp2: ---");
LCD_OutChar(0xdf);
LCD_OutString("C");

Timer_mwait(1000);

  SCI_OutString("Beginning");   OutCRLF();

for(;;) {
    InputTimeoutFlag=0;
    LCDCommand(0x80+0x4F);
LCD_OutString("I");

    MainSelection=SCI_InUDec3();     
    
    if(MainSelection==66){
    //SCI_OutString("TIMEOUT!!!!!!!!!!!");
    }

    if(MainSelection==8){
        OutCRLF();
        SCI_OutString("--------------------------------------------------------");   OutCRLF();
        SCI_OutString("1 - Display Current Settings");   OutCRLF();
        SCI_OutString("2 - Change Current Settings");   OutCRLF();
        SCI_OutString("3 - Transmit Settings");   OutCRLF();
        SCI_OutString("4 - Display Temperature");   OutCRLF();
        SCI_OutString("Choice :");   OutCRLF();
    }

    if(MainSelection==1){
      OutputWrite2();
    }

    if(MainSelection==2){
      AmpSettingsInput();
    }

    if(MainSelection==3) {
      TransmitControl();
    }

    if(MainSelection==4) {
        for(tempiterations=1;tempiterations<30;tempiterations++){
          Temperature();    
        
        }
    }

    if(MainSelection==5){
          while((SCISR1 & 0x80) == 0){};
          SCIDRL = Temp1Global;
          while((SCISR1 & 0x80) == 0){};
          SCIDRL = Temp2Global;
    //      Timer_mwait(3000);       
    }

    
    //Output Settings
    ii=0;jj=0;kk=0;
    if(MainSelection==6) {
      Timer_mwait(1000);
       for(ii=0;ii<=15;ii++){
        for(jj=0;jj<=2;jj++){ 
          for(kk=0;kk<=1;kk++){
              while((SCISR1 & 0x80) == 0){};
              SCIDRL = OverallReceived[ii][jj][kk];

          }
        }
       }
     //Timer_mwait(5000);
     }
     
   
    
    //Input Settings
    ii=0;jj=0;kk=0;
    if(MainSelection==7) {
       for(ii=0;ii<=15;ii++){
        for(jj=0;jj<=2;jj++){ 
          for(kk=0;kk<=1;kk++){
             while((SCISR1 & 0x20) == 0){};
             OverallSettings[ii][jj][kk] = SCIDRL;

             while((SCISR1 & 0x80) == 0){};
             SCIDRL = OverallSettings[ii][jj][kk];
              
          }
        }
       }
     //Timer_mwait(500);
     }



}




}

  




//----------------------------------------------------------
unsigned short transmitSPIOld(unsigned short transnum){  
  unsigned short dummySPISR,SPIDRValue;
  
  
  
  dummySPISR=SPISR;
  SPIDR=transnum;
  while((SPISR&0x20)==0);  //Wait until data has been sent
  while((SPISR&0x80)==0);  //Wait until data has been received
  SPIDRValue=SPIDR;        //Read received Value
  dummySPISR=SPISR;        //Read SPISR to clear register 
  return SPIDRValue ;      //Return the received value
}


//----------------------------------------------------------
unsigned short transmitSPI(unsigned short transnum){  
  unsigned short dummySPISR,SPIDRValue,bit,data[8], dataOutput[8],registervalue;
  //SPICR1 |= 0x40;  //Disable SPI
  //SCI_OutString("Start ");   OutCRLF();
  //DDRM=0x04; // Set direction of pins  
  //PTM=0x10; //all pins high
  //PTM=0x00; //all pins low
  //CRGINT = 0x80; // RTIE=1 enable rti
  //RTICTL = 0x7F; // Interrupt Every 0.131seconds  
  //RTITrigger=0;
  
  
  if(transnum>=128){
  data[7]=1;transnum=transnum-128;
  } else{ data[7]=0;
  }
    if(transnum>=64){
  data[6]=1;transnum=transnum-64;
  }  else{ data[6]=0;
  }
    if(transnum>=32){
  data[5]=1;transnum=transnum-32;
  } else{ data[5]=0;
  }
    if(transnum>=16){
  data[4]=1;transnum=transnum-16;
  }    else{ data[4]=0;
  }
    if(transnum>=8){
  data[3]=1;transnum=transnum-8;
  }  else{ data[3]=0;
  }
    if(transnum>=4){
  data[2]=1;transnum=transnum-4;
  }  else{ data[2]=0;
  }
    if(transnum>=2){
  data[1]=1;transnum=transnum-2;
  } else{ data[1]=0;
  }
    if(transnum>=1){
  data[0]=1;transnum=transnum-1;
  } else{ data[0]=0;
  }
  
  //data[0]=1;
  //data[1]=1;
  //data[2]=1;  
  //data[3]=0;  
  //data[4]=1;  
  //data[5]=1;
  //data[6]=0;
  //data[7]=1;
  
          
  TSCR1 = 0x80;   // Enable TCNT, 8MHz in run modes
  TSCR2 = 0x00;   // divide by 8 TCNT prescale, TCNT at 1usec
  PACTL = 0;
 
  PTM |= 0x20;//SCK High 
  Timer_Fast(10);
  
  //PTT &= ~0x80; //SYNC Low for beginning of transmission
  
  for(bit=1;bit<9;bit++){  
    
     if(data[8-bit]==1){
        PTM |= 0x10;
      } else{
          PTM &= ~0x10;
      }    
      
      
      Timer_Fast(10);
      
      PTM &= ~0x20;//SCK Low  
      
              
      Timer_Fast(2);
      registervalue=PTM;
      if((registervalue&=0x04) == 0x04)
      {dataOutput[8-bit]=1;}
      else {dataOutput[8-bit]=0;}//**Read in a bit
      
      Timer_Fast(10);
      PTM |= 0x20;//SCK High
      Timer_Fast(2);

  }
  Timer_Fast(10);
  
  //Reset the timer values to their normal      
  TSCR1 = 0x80;   // Enable TCNT, 8MHz in run modes
  TSCR2 = 0x02;   // divide by 8 TCNT prescale, TCNT at 1usec
  PACTL = 0; 
  SPIDRValue=dataOutput[0]*1+dataOutput[1]*2+dataOutput[2]*4+dataOutput[3]*8+dataOutput[4]*16+dataOutput[5]*32+dataOutput[6]*64+dataOutput[7]*128;
  //SCI_OutUDec(SPIDRValue);
  return SPIDRValue;
  
}


//-------------------------------------------------
void PrintOutSettings(void){
    unsigned int Location;
    OutCRLF();
    SCI_OutString("Coil, Axis, Switch 1, Switch 2");OutCRLF();
    
    for(Location = 0; Location < 16;Location++){
      SCI_OutUDec(Location+1);SCI_OutString(": x-");SCI_OutUDec(OverallSettings[Location][0][0]);SCI_OutString(", ");SCI_OutUDec(OverallSettings[Location][0][1]);SCI_OutString(": y-");SCI_OutUDec(OverallSettings[Location][1][0]);SCI_OutString(", ");SCI_OutUDec(OverallSettings[Location][1][1]);SCI_OutString(": z-");SCI_OutUDec(OverallSettings[Location][2][0]);SCI_OutString(", ");SCI_OutUDec(OverallSettings[Location][2][1]);OutCRLF();    
    
    }
}




//----------------------------------------------------------------------------
void OutputWrite2(void){
int i,j;
unsigned char LP1Setting,LP2Setting,HP1Setting,HP2Setting, Gain1Setting,Gain2Setting,Gain3Setting, AASetting;
unsigned char currentvalue,currentvalue2;

OutCRLF();
SCI_OutString("----|---------|----------|-----------|-----------|-----------|------|");OutCRLF();
SCI_OutString("Coil|");SCI_OutString(" 5kHz LP |"); SCI_OutString(" 10kHz LP |"); SCI_OutString(" 100kHz HP |");SCI_OutString(" 300kHz HP |");SCI_OutString(" AntiAlias |");SCI_OutString(" Gain |");OutCRLF();    

for(i=0;i<16;i++){
  for(j=0;j<3;j++){
    currentvalue=OverallSettings[i][j][0];
    currentvalue2=OverallSettings[i][j][1]; 

//1st Switch Chip
    if(currentvalue>=128){HP2Setting=1;currentvalue=currentvalue-128;}
    else if(currentvalue>=64){HP2Setting=0;currentvalue=currentvalue-64;} 
    else{SCI_OutString("ERROR1  ");}
    
    if(currentvalue>=32){HP1Setting=1;currentvalue=currentvalue-32;}
    else if(currentvalue>=16){HP1Setting=0;currentvalue=currentvalue-16;} 
    else{SCI_OutString("ERROR2  ");}

    if(currentvalue>=8){LP2Setting=1;currentvalue=currentvalue-8;}
    else if(currentvalue>=4){LP2Setting=0;currentvalue=currentvalue-4;} 
    else{SCI_OutString("ERROR3  ");}

    if(currentvalue>=2){LP1Setting=1;currentvalue=currentvalue-2;}
    else if(currentvalue>=1){LP1Setting=0;currentvalue=currentvalue-1;} 
    else{SCI_OutString("ERROR4  ");}

//Second Switch Chip    
    if(currentvalue2>=128){Gain1Setting=1;currentvalue2=currentvalue2-128;}
    else{Gain1Setting=0;}
    
    if(currentvalue2>=64){Gain2Setting=1;currentvalue2=currentvalue2-64;} 
    else{Gain2Setting=0;}
    
    if(currentvalue2>=32){Gain3Setting=1;currentvalue2=currentvalue2-32;}
    else{Gain3Setting=0;}
    
    if(currentvalue2>=16){AASetting=0;currentvalue2=currentvalue2-16;}
    else if(currentvalue2>=8){AASetting=1;currentvalue2=currentvalue2-8;} 
    else{SCI_OutString("ERROR5  ");}

    if(currentvalue2>=4){SCI_OutString("ERROR6  ");}
    if((Gain1Setting==0)&(Gain2Setting==0)&(Gain3Setting==0)){SCI_OutString("ERROR7  ");}
  
    SCI_OutString(" ");SCI_OutUDec(i+1);SCI_OutString(",");SCI_OutUDec(j+1);SCI_OutString("|     ");SCI_OutUDec(LP1Setting);SCI_OutString("   |    ");SCI_OutUDec(LP2Setting);SCI_OutString("     |     ");SCI_OutUDec(HP1Setting);SCI_OutString("     |     ");SCI_OutUDec(HP2Setting);SCI_OutString("     |     ");SCI_OutUDec(AASetting);SCI_OutString("     |   ");SCI_OutUDec(Gain1Setting);;SCI_OutUDec(Gain2Setting);SCI_OutUDec(Gain3Setting);SCI_OutString("  |");OutCRLF();
  
  }
}

}





//--------------------------------------------------------------------------
void AmpSettingsInput(void){

  unsigned short contnum=0, number1=0,SwitchChip1=0, SwitchChip2=0,temp1=0,CoilNumberTemp=0,axistemp=0;
  char character;
  int i,j;
  
  OutCRLF();SCI_OutString("Enter 10 1-digit numbers for amp settings CoilNum(2dig);axis (xyz);HP1;HP2;LP1;LP2;AA;G1;G2;G3 (no enter): ");

//Coil Number digit 1
      character = 'm';	

while(character != CR){
      
      character = SCI_InChar();

      if(character=='a'){CoilNumberTemp=0;axistemp=0;SCI_OutChar(character);} 
      else{
        //Coil Number Digit 1
        temp1=(character-'0');SCI_OutChar(character);CoilNumberTemp=temp1*10;

        //Coil Number Digit 2
        character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
        CoilNumberTemp=CoilNumberTemp+temp1;

        //Coil Axis
        character = SCI_InChar();	
        if (character=='x'){axistemp=1;}
        if (character=='y'){axistemp=2;}
        if (character=='z'){axistemp=3;}
        SCI_OutChar(character);
      }

/*
Switch  NumActivate Chip1 Chip2
S1      1           LP1_0 x
S2      2           LP1_1 x
S3      4           LP2_0 x
S4      8           LP2_1 AA_1
S5      16          HP1_0 AA_0
S6      32          HP1_1 Gain_High
S7      64          HP2_0 Gain_Med
S8      128         HP2_1 Gain_Low
*/  
 
//HP1 Filter
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      if (temp1==0){SwitchChip1=1;}
      if (temp1==1){SwitchChip1=2;}
   
//HP2 Filter
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      if (temp1==0){SwitchChip1=SwitchChip1 + 4;}
      if (temp1==1){SwitchChip1=SwitchChip1 + 8;}

//LP1 Filter      
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      if (temp1==0){SwitchChip1=SwitchChip1 + 16;}
      if (temp1==1){SwitchChip1=SwitchChip1 + 32;}
      
//LP2 Filter      
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      if (temp1==0){SwitchChip1=SwitchChip1 + 64;}
      if (temp1==1){SwitchChip1=SwitchChip1 + 128;}
      
//AA Filter            
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      //if (temp1==0){SwitchChip2= 16;}
      //if (temp1==1){SwitchChip2= 8;}
      if (temp1==0){SwitchChip2= 8;}
      if (temp1==1){SwitchChip2= 4;}
 
 //Gain Resistor 1   
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      //if (temp1==1){SwitchChip2=SwitchChip2 + 128;}
      if (temp1==1){SwitchChip2=SwitchChip2+64;}
 
 //Gain Resistor 2
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      //if (temp1==1){SwitchChip2=SwitchChip2 + 64;}
      if (temp1==1){SwitchChip2=SwitchChip2+0;}
 
 //Gain Resistor 3
      character = SCI_InChar();temp1=(character-'0');SCI_OutChar(character);
      //if (temp1==1){SwitchChip2=SwitchChip2 + 32;}
      if (temp1==1){SwitchChip2=SwitchChip2+128;}

//Either set all of the values or individual ones depending on whether a was set
if(CoilNumberTemp==0){
  for(i=0;i<16;i++){
     for(j=0;j<3;j++){
      OverallSettings[i][j][0]= SwitchChip1;
     OverallSettings[i][j][1]=  SwitchChip2;
    }
  }
} else{
  
    OverallSettings[CoilNumberTemp-1][axistemp-1][0]= SwitchChip1;
    OverallSettings[CoilNumberTemp-1][axistemp-1][1]= SwitchChip2;
}
   

      character = SCI_InChar();SCI_OutChar(character);
      //character = SCI_InChar();		
      
}
OutCRLF();SCI_OutString("Coil Number :  "); SCI_OutUDec(CoilNumberTemp);SCI_OutString("  |  Switch Chip1 :  "); SCI_OutUDec(SwitchChip1) ;SCI_OutString("   | Switch Chip2 :  "); SCI_OutUDec(SwitchChip2);OutCRLF();
OutCRLF();PrintOutSettings();OutCRLF(); 
    return;
  
}




//------------------------------------------------------
void TransmitControl(void){
    unsigned short TransmitSelection,Count,Errors,Previous1,Previous2,Previous3,DoubleTransmit,Trans1,Trans2,MaxSend,Received1=0,Var1,Var2,Var3,Received2=0,Received3=0,dummy6=0;
    int k,m;   
    unsigned short dummyValue3,dummyValue4,dummyValue1,dummyValue2; 
    TransmitSelection=SCI_InUDec(); 
    //OutCRLF();


LCDCommand(0x80+0x4F);
LCD_OutString("T");



    if(TransmitSelection==8){
    OutCRLF(); 
    SCI_OutString("1 - Transmit a single number ");OutCRLF();  
    SCI_OutString("2 - Transmit The entire Sequence ");OutCRLF();  
    SCI_OutString("3 - Transmit in doubles up to a value ");OutCRLF();  
    SCI_OutString("4 - Transmit Two Seperate Numbers");OutCRLF();
    SCI_OutString("5 - Transmit Combinations of Seperate Numbers");OutCRLF();
    }

    if(TransmitSelection==1){
          PrintOutSettings();
          SCI_OutString(" Enter a number to transmit twice: ");  DoubleTransmit=SCI_InUDec(); 
          PTT &= ~0x80; //SYNC Low for beginning of transmission
          Received1=transmitSPI(DoubleTransmit);       
          Received2=transmitSPI(DoubleTransmit);
          SCI_OutString("--Transmitted Number x 2 :"); SCI_OutUDec(DoubleTransmit); SCI_OutString(" | -- Received numbers after Transmission : ");SCI_OutUDec(Received1);SCI_OutString(" ");SCI_OutUDec(Received2);OutCRLF();
          PTT|=0x80; //SYNC High at end of transmission 
    }

 

    if(TransmitSelection==2){
      OutCRLF();
      PrintOutSettings();
      OutCRLF();
      
      PTT &= ~0x80; //SYNC Low for beginning of transmission
      for(k=0;k<16;k++){
        for(m=0;m<3;m++){
          dummyValue3=OverallSettings[15-k][2-m][1];
          dummyValue1=transmitSPI(dummyValue3);
          dummyValue4=OverallSettings[15-k][2-m][0];       
          dummyValue2=transmitSPI(dummyValue4);
          OverallReceived[15-k][2-m][1]=dummyValue1;
          OverallReceived[15-k][2-m][0]=dummyValue2;
          dummy6=dummy6+1;
          SCI_OutUDec(dummy6);SCI_OutString(" Transmitted Numbers: ");SCI_OutUDec(dummyValue3);SCI_OutString(",");SCI_OutUDec(dummyValue4);SCI_OutString(" | Received numbers after Transmission: ");SCI_OutUDec(OverallReceived[k][m][1]);SCI_OutString(" ");SCI_OutUDec(OverallReceived[k][m][0]);OutCRLF();
        }
      }
      PTT |= 0x80; //SYNC High at end of transmission 
    }

    //Auto Transmit
    if(TransmitSelection==6){
      PTM |= 0x20;//SCK High
      Timer_Fast(10);
      PTT &= ~0x80; //SYNC Low for beginning of transmission 0 OLD SYSTEM
      for(k=0;k<16;k++){
        for(m=0;m<3;m++){
          dummyValue3=OverallSettings[15-k][2-m][1];
          dummyValue1=transmitSPI(dummyValue3);
          dummyValue4=OverallSettings[15-k][2-m][0];       
          dummyValue2=transmitSPI(dummyValue4);
          OverallReceived[15-k][2-m][1]=dummyValue1;
          OverallReceived[15-k][2-m][0]=dummyValue2;
          dummy6=dummy6+1;
        }
      }
      //PTT |= 0x80; //SYNC High at end of transmission OLD SYSTEM
      while((SCISR1 & 0x80) == 0){};
      
      SCIDRL = 1; //Transmit Success
      //Timer_mwait(5000);
    }

 
 
    if(TransmitSelection==3){
      OutCRLF();
     SCI_OutString(" Enter a number to transmit in triples up to : ");  MaxSend=SCI_InUDec(); 
      k=0;
      while((k<(MaxSend+1))) {
        PTT &= ~0x80; //SYNC Low for beginning of transmission
              Received1=transmitSPI(k);       
              Received2=transmitSPI(k);
              Received3=transmitSPI(k);
        PTT |= 0x80; //SYNC High at end of transmission 
        OutCRLF();
        SCI_OutString("Transmitted Number x 3 :"); SCI_OutUDec(k); SCI_OutString(" | Received numbers after Transmission : ");SCI_OutUDec(Received1);SCI_OutString(" ");SCI_OutUDec(Received2);SCI_OutString(" ");SCI_OutUDec(Received3); 
        k=k+1;
      }
    }


 
    
   
    if(TransmitSelection==4){
          PrintOutSettings();
          SCI_OutString(" Enter a number to transmit : ");  Trans1=SCI_InUDec(); 
          SCI_OutString(" Enter a number to transmit : ");  Trans2=SCI_InUDec(); 
          PTT &= ~0x80; //SYNC Low for beginning of transmission
        for(k=0;k<20;k++){
          Received1=transmitSPI(Trans1);       
          Received2=transmitSPI(Trans2);
          SCI_OutString("--Transmitted Numbers :"); SCI_OutUDec(Trans1);SCI_OutString(", "); SCI_OutUDec(Trans2); SCI_OutString(" | -- Received numbers after Transmission : ");SCI_OutUDec(Received1);SCI_OutString(" ");SCI_OutUDec(Received2);OutCRLF();

        }
          PTT|=0x80; //SYNC High at end of transmission 

          SCI_OutString("--Transmitted Numbers :"); SCI_OutUDec(Trans1);SCI_OutString(", "); SCI_OutUDec(Trans2); SCI_OutString(" | -- Received numbers after Transmission : ");SCI_OutUDec(Received1);SCI_OutString(" ");SCI_OutUDec(Received2);OutCRLF();
    }
   
   Errors=0;
   Count=0;
   if(TransmitSelection==5){
          SCI_OutString(" Enter a max for number 1 : ");  Trans1=SCI_InUDec(); 
          SCI_OutString(" Enter a max for number 2 : ");  Trans2=SCI_InUDec();  
          
          for(Var1=1;Var1<Trans1+1;Var1++){
            for(Var2=1;Var2<Trans2+1;Var2++){
            PTT &= ~0x80; //SYNC Low for beginning of transmission  
            Received1=transmitSPI(Var1);       
            Received2=transmitSPI(Var2);
            Var3=Var2;
            Received3=transmitSPI(Var3);
            PTT|=0x80; //SYNC High at end of transmission 
            //SCI_OutString("Transmitted Numbers :"); SCI_OutUDec(Var1);SCI_OutString(","); SCI_OutUDec(Var2);SCI_OutString(","); SCI_OutUDec(Var3); SCI_OutString(" |Received: ");SCI_OutUDec(Received1);SCI_OutString(",");SCI_OutUDec(Received2);SCI_OutString(",");SCI_OutUDec(Received3);OutCRLF();           
            if((Received1==Previous1)&&(Received2==Previous2)&&(Received3==Previous3)){
            Count=Count+1;}
            else{
            Errors=Errors+1;
            OutCRLF();SCI_OutString("Error");OutCRLF();
            }
            
            Previous1=Var1;
            Previous2=Var2;
            Previous3=Var3;
            }
          }
   
     SCI_OutString("-Number of Errors : "); SCI_OutUDec(Errors);SCI_OutString("-Number of Success : "); SCI_OutUDec(Count); SCI_OutString("-Total :"); SCI_OutUDec(Trans1*Trans2);
   }

}

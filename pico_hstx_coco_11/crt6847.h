//Simple-Circuit 2026
//crt6847 is a 6847 emulator project for educational purposes.
//It receives 8-bit data from the serial port at 3MHz baud
//D4 is UART2 out, D5 is UART2 in
//up to 6114 bytes of data are converted to coco alpha or graphics modes
//Encoded as 332 color encoded RGB bytes in a 512x384 pixel area. The display scan is 640x480

/*MIT License

crt6847.h Copyright (c) 2026 simple-circuit

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#define CGRN 0b00010000
#define CBLK 0b00000000
#define CYEL 0b10010000
#define CBLU 0b00000010
#define CRED 0b10000000
#define CBUF 0b01101100
#define CCYN 0b00010010
#define CMAG 0b10000010
#define CORG 0b11110000
#define CWHT 0b11111111

volatile uint32_t bc[256][12];    //character pixel matrix for ascii modes
volatile uint32_t coco_mem[6144]; //6K of coco video memory
  

void setup() {

#include "matrix.h"  //get ascii dot matrix
 volatile int baudrate = 3000000; //max baud rate for 250MHz and 200MHz clocks
 pinMode(0,INPUT_PULLUP);
 pinMode(1,INPUT_PULLUP);
 pinMode(2,INPUT_PULLUP);
 pinMode(4,OUTPUT);
 pinMode(8,OUTPUT);        //not used timing test out
 pinMode(9,OUTPUT);        //not used timing test out
 pinMode(10,OUTPUT);       //not used DVI timing check
 Serial2.setRX(5);         //Serial I/O is on pins 4 and 5
 Serial2.setTX(4);
 Serial2.setFIFOSize(8192); //serial in storage 6K+ of video data
 Serial2.begin(baudrate); 
}

void loop(){
  volatile unsigned int i,k,m,ii,jj,v,gmode,data_len;
  volatile int j,n,xp,yp,x1,y1,x2,y2,h;
  volatile unsigned int shft;
  volatile char c,c2,xx;
  volatile char c0,c1,css;
  volatile char pal[] = {CGRN,CYEL,CBLU,CRED,CBUF,CCYN,CMAG,CORG}; //eight color palette
  volatile char pal2[] = {CGRN,CYEL,CBLU,CRED}; //four color palette css 0
  volatile char pal3[] = {CBUF,CCYN,CMAG,CORG}; //four color palette css 1
  
   i=0;
   gmode = 0; //default text mode = 0
   for (i=0; i<256;i++) coco_mem[i]=i & 0x20;   //Fill video text with spaces
   for (i=256;i<6144;i=i+31) coco_mem[i]=0x00;  //clear graphics  
   for (j=0; j<0x40;j++) for (i=9;i<12;i++) bc[j][i] = 0x00; //creat gaps in 5x7 character
   for (j=0; j<0x40;j++) for (i=0;i<2;i++) bc[j][i] = 0x00;  //for 8x12 area
   for (j=0;j<307200;j++) framebuf[j] = 0x25; //boarder color
 
while(true){ 
    
    // gpio_put(8,HIGH);            //used for timing checks
     // look for record start
    i =0;
     while (i<5){                     //look for 0xff 0x00 0xaa 0x55 0x81 sequence the exit 
      while (Serial2.available() == 0);  
      c = Serial2.read();
      if (c==0xFF) i=1;               //restart state if 0xff found
      if ((c==0x00) && (i==1)) i++;   //advance on correct next
      if ((c==0xAA) && (i==2)) i++; 
      if ((c==0x55) && (i==3)) i++; 
      if ((c==0x81) && (i==4)) i++; 
     }   
      while (Serial2.available() == 0);  //get mode number 0 to 12
      gmode = Serial2.read();
      while (Serial2.available() == 0);  //get msb of data record length
      data_len = Serial2.read()<<8;
      while (Serial2.available() == 0);  //get LSB of data record length
      data_len = Serial2.read() + data_len;
      if (data_len > 6144) data_len=6144; //don't overflow memory matrix
      for (i=0; i<data_len;i++){
        while (Serial2.available() == 0); //may not need a timeout break here
        coco_mem[i]=Serial2.read();       //the mem_capture processor
      }                                   //sends 6K every 50ms by default    
      i=0;
     for (j=0;j<4;j++){                   //test tail for 0x00 0x55 0xff 0xaa sequence
      while (Serial2.available() == 0);  
      c = Serial2.read();
      if ((c==0x00)&&(i==0)) i++;
      if ((c==0x55) && (i==1)) i++; 
      if ((c==0xFF) && (i==2)) i++;     
      if ((c==0xAA) && (i==3)) i++; 
     }   
   css = gmode & 0x80;       //css color bit received as bit7
   gmode = gmode & 0x0F;     //0-12 is the gmode masked to 4 bits    
   if (i < 4) gmode = 99;    //flag if bad read and don't update display
   
   Serial2.write(byte(13)); //trigger sent but mem_capture does not receive?
   
   if (gmode==0) { //alpha numerics and Semigraphics-4
   c0 = CBLK;
   if (css==0) c1 = CGRN; else c1 = CORG; //set the css color
   //Write matrix characters to screen
   for (yp = 0; yp<16; yp++){  //verticle lines 16 loops
    for (k=0; k<12; k++){      //12 pixel high character loop
     for (xp = 0; xp<32; xp++){ //32 character screen width loop
      for (jj=0;jj<1280;jj=jj+640){ //double the vertical lines, 640 pixels per line
       n =(yp*15360) + (k*1280) + (xp*16)+ 30784 + jj;  //n holds the framebuf[] location to set
       c =  coco_mem[xp+(yp*32)]; //c is data byte from the coco video text storage array
       if (c&0b10000000){         //b7 is the semigraphics mode
         c1 = pal[(c>>4) & 7];    //b6 b5 b4 hold the color value
         xx =  0;                 //xx is the exclusive or value for inverted text
         c = c & 0b10001111;      //mask for character code, still using 12x8 grid for display 
       } else {
        if (css==0) c1 = CGRN; else c1 = CORG; 
        if (c&0b01000000) xx = 0xFF; else xx = 0; //invert if b6 set
        c = c & 0b00111111;
       }
       m = bc[c][k] ^ xx;       
       shft = 0x080;
       if ((m & shft)==0){        //b7 if set, color 0
        framebuf[n++] = c0;       //save and increment twice to double width
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;      //else color1
        framebuf[n++] = c1;
       }                         
       shft = shft>>1;          //0x80 --> 0x40 for next bit detection
       if ((m & shft)==0){      //b6
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b5 
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b4
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b3
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b2
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){   //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }             
     }
     }
     
    }
   }
  }
  if ((gmode==1) && (gpio_get(1)==0)) gmode = 2;
  if (gmode==1){ //256x192 two color graphics
   c0 = CBLK;
   if (css==0) c1 = CGRN; else c1 = CORG;
   //Write data bits to screen mem 
    for (yp = 0; yp<192; yp++){  //192 pixels high
     for (xp = 0; xp<32; xp++){  //32x8= 258 pixels wide
      for (jj=0;jj<1280;jj=jj+640){ //double line width, each line is 640 pixels
       n =(yp*1280) + (xp*16)+ 30784 + jj;  //get screen location, UL = 48 lines down and 64 pixels over   
       m=coco_mem[xp+(yp*32)];  //find coco graphics pixel value             
       shft = 0x080;            //check each bit using left shift of test bit
       if ((m & shft)==0){        //b7
        framebuf[n++] = c0;      //double the pixel width
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b6
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b5 
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b4
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b3
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b2
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }             
     }         
    }
   }
  }
  
  if (gmode==2){  //128x192 four color graphics
      if(css==0) {
        pal2[0] = CGRN;
        pal2[1] = CYEL;
        pal2[2] = CBLU;
        pal2[3] = CRED;
      } else {
        pal2[0] = CBUF;
        pal2[1] = CCYN;
        pal2[2] = CMAG;
        pal2[3] = CORG;    
      }  
     if (gpio_get(1)==0){
        pal2[0] = CBLK;
        pal2[1] = CBLU;
        pal2[2] = CORG;
        pal2[3] = CWHT;     
     }
    for (yp = 0; yp<192; yp++){  //192 lines
     for (xp = 0; xp<32; xp++){  //8x32/2=128 wide using 2-pixels for color
      for (jj=0;jj<1280;jj=jj+640){ //double lines
       n =(yp*1280) + (xp*16)+ 30784 + jj;   //find the location of the pixel in framebuf[]  
       m=coco_mem[xp+(yp*32)];   //get the data byte from coco video mem    
       
       c0 = pal2[(m>>6)&3];         //b7b6 //shift to mask at 3 bits, lookup in 4-color table
       framebuf[n++] = c0;          //increase pixel width 4x
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>4)&3];         //b5b4
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>2)&3];         //b3b2
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[m & 3];         //b1b0
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
     }         
    }
   }
  }
  
  if (gmode==3){        //128x192 two color graphics
    c0 = CBLK;
    if (css==0) c1 = CGRN; else c1 = CBUF;
    for (yp = 0; yp<192; yp++){  
     for (xp = 0; xp<16; xp++){   //was xp<32
      for (jj=0;jj<1280;jj=jj+640){ 
       n =(yp*1280) + (xp*32)+ 30784 + jj;    //was xp*16 
       m=coco_mem[xp+(yp*16)];       
       shft = 0x080;
       if ((m & shft)==0){        //b7
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b6
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b5 
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b4
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b3
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b2
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; //b0
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }             
     }         
    }
   }
  }
  
   if (gmode==4){  //128x96 four color graphics
      if(css==0) {
        pal2[0] = CGRN;
        pal2[1] = CYEL;
        pal2[2] = CBLU;
        pal2[3] = CRED;
      } else {
        pal2[0] = CBUF;
        pal2[1] = CCYN;
        pal2[2] = CMAG;
        pal2[3] = CORG;    
      }  
    for (yp = 0; yp<96; yp++){  
     for (xp = 0; xp<32; xp++){
      for (jj=0;jj<2560;jj=jj+640){         //was jj<1280
       n =(yp*2560) + (xp*16)+ 30784 + jj;  //was yp*1280   
       m=coco_mem[xp+(yp*32)];       
       
       c0 = pal2[(m>>6)&3];         //b7b6
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>4)&3];         //b5b4
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>2)&3];         //b3b2
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[m & 3];         //b1b0
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
     }         
    }
   }
  }

  if (gmode==5){        //128x96 two color graphics
   c0 = CBLK;
   if (css==0) c1 = CGRN; else c1 = CBUF;
    for (yp = 0; yp<96; yp++){  
     for (xp = 0; xp<16; xp++){  
      for (jj=0;jj<2560;jj=jj+640){           
       n =(yp*2560) + (xp*32)+ 30784 + jj;    
       m=coco_mem[xp+(yp*16)];       
       shft = 0x080;
       if ((m & shft)==0){        //b7
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b6
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b5 
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b4
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b3
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b2
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; //b0
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }             
     }         
    }
   }
  }

   if (gmode==6){  //128x64 four color graphics
      if(css==0) {
        pal2[0] = CGRN;
        pal2[1] = CYEL;
        pal2[2] = CBLU;
        pal2[3] = CRED;
      } else {
        pal2[0] = CBUF;
        pal2[1] = CCYN;
        pal2[2] = CMAG;
        pal2[3] = CORG;    
      }  
    for (yp = 0; yp<64; yp++){  //was 96
     for (xp = 0; xp<32; xp++){
      for (jj=0;jj<3840;jj=jj+640){         //was jj<2560
       n =(yp*3840) + (xp*16)+ 30784 + jj;  //was yp*2560   
       m=coco_mem[xp+(yp*32)];       
       
       c0 = pal2[(m>>6)&3];         //b7b6
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>4)&3];         //b5b4
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>2)&3];         //b3b2
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[m & 3];         //b1b0
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
     }         
    }
   }
  }

  if (gmode==7){        //128x64 two color graphics
   c0 = CBLK;
   if (css==0) c1 = CGRN; else c1 = CBUF;
    for (yp = 0; yp<64; yp++){  //was 96
     for (xp = 0; xp<16; xp++){  
      for (jj=0;jj<3840;jj=jj+640){           
       n =(yp*3840) + (xp*32)+ 30784 + jj;    
       m=coco_mem[xp+(yp*16)];       
       shft = 0x080;
       if ((m & shft)==0){        //b7
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b6
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b5 
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b4
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b3
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b2
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; 
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){    //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1; //b0
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }             
     }         
    }
   }
  }

   if (gmode==8){  //64x64 four color graphics
      if(css==0) {
        pal2[0] = CGRN;
        pal2[1] = CYEL;
        pal2[2] = CBLU;
        pal2[3] = CRED;
      } else {
        pal2[0] = CBUF;
        pal2[1] = CCYN;
        pal2[2] = CMAG;
        pal2[3] = CORG;    
      }  
    for (yp = 0; yp<64; yp++){  //was 96
     for (xp = 0; xp<16; xp++){  //was 32
      for (jj=0;jj<3840;jj=jj+640){         //was jj<2560
       n =(yp*3840) + (xp*32)+ 30784 + jj;  //was yp*2560   
       m=coco_mem[xp+(yp*16)];       
       
       c0 = pal2[(m>>6)&3];         //b7b6
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>4)&3];         //b5b4
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[(m>>2)&3];         //b3b2
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;

       c0 = pal2[m & 3];         //b1b0
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
       framebuf[n++] = c0;
     }         
    }
   }
  }


  if (gmode==9){        //64x48 four color semigraphics
      if(css==0) {
        pal2[0] = CGRN;
        pal2[1] = CYEL;
        pal2[2] = CBLU;
        pal2[3] = CRED;
      } else {
        pal2[0] = CBUF;
        pal2[1] = CCYN;
        pal2[2] = CMAG;
        pal2[3] = CORG;    
      }
   c0 = CBLK;
    for (yp = 0; yp<16; yp++){  
    for (jj=0;jj<5120;jj=jj+640){           //8 loops per vertical pixel 
     for (xp = 0; xp<32; xp++){  
      shft = 0b00100000; 
       n =(yp*5120) + (xp*16)+ 30784 + jj;    
       m=coco_mem[xp+(yp*32)];       
       c1 = pal2[m>>6];   
       if ((m & shft)==0){        //b1
        framebuf[n++] = c0;       //8 repeats per horizontal pixel
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }
       shft = shft >> 1;       
     }         
    }
   }
  }

  if (gmode==10){        //64x64 eight color semigraphics
   c0 = CBLK;
   x2=-1;
    for (yp = 0; yp<64; yp++){  
     x2++;
     if (x2>3) x2=0;      
    for (jj=0;jj<3840;jj=jj+640){  
     for (xp = 0; xp<32; xp++){  
      if (x2>1) shft = 0b00000010; else shft = 0b00001000;              
       n =(yp*3840) + (xp*16)+ 30784 + jj;    //6 vertical repeats
       m=coco_mem[xp+(yp*32)];       
       c1 = pal[(m>>4) & 7];   

       if ((m & shft)==0){        //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }
       shft = shft >> 1;       
     }         
    }
   }
  }

  if (gmode==11){        //64x96 eight color semigraphics
   c0 = CBLK;
    x2=-1;
    for (yp = 0; yp<96; yp++){
      x2++;
      if (x2>5) x2=0;
      for (jj=0;jj<2560;jj=jj+640){  
      for (xp = 0; xp<32; xp++){    
       if (x2>2) shft = 0b00000010; else shft = 0b00001000;         
       n =(yp*2560) + (xp*16)+ 30784 + jj;    //4 vertical repeats
       m=coco_mem[xp+(yp*32)];       
       c1 = pal[(m>>4) & 7];   

       if ((m & shft)==0){        //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }
       shft = shft >> 1;       
     }         
    }
   }
  }

  if (gmode==12){        //64x192 eight color semigraphics
   c0 = CBLK;
    x2=-1;
    for (yp = 0; yp<192; yp++){ 
      x2++;  
      if (x2>12) x2=0;        
     for (jj=0;jj<1280;jj=jj+640){ 
      for (xp = 0; xp<32; xp++){      
       if (x2>5) shft = 0b00000010; else shft = 0b00001000;         
       n =(yp*1280) + (xp*16)+ 30784 + jj;    //2 vertical repeats
       m=coco_mem[xp+(yp*32)];       
       c1 = pal[(m>>4) & 7];   

       if ((m & shft)==0){        //b1
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }       
       shft = shft>>1;
       if ((m & shft)==0){      //b0
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
        framebuf[n++] = c0;
       }else{
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
        framebuf[n++] = c1;
       }
       shft = shft >> 1;       
     }         
    }
   }
  }


     
   // gpio_put(8,LOW);
    if (Serial2.available()==0) Serial2.write(byte(13)); //try again if no trigger
 }
} 

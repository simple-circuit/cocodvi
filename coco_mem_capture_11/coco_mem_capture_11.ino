//coco_mem_capture
//captures all data writes to coco 2 memory from the cartridge port 
//Board: Raspberry-Pi Pico-2 using compile options: No Fs, 200MHz, -O optimize, No USB

/*MIT License

coco_mem_capture Copyright (c) 2026 simple-circuit

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


volatile uint32_t ccmem[65536];  //image of the coco's 64K of memory space
volatile uint32_t x,a;
volatile uint32_t sam_reg[256];  //the sam does not use data but set resets by address space 
volatile uint32_t dmc = 0;       //the current display mode control bits in the sam 
volatile uint32_t gmode = 0;     //program specific gaphics mode 0 to 12
volatile uint32_t pia = 0;       //data at address 0xFF22
volatile uint32_t doffset = 0x400;  //the sam chip display offset
volatile uint32_t data_len = 1;     //record length is actually fixed a 6144 bytes
volatile uint32_t vid_size[] = {512,1024,2048,1536,3072,3072,6144,512};  //for variable records.. not used
volatile uint32_t i,j;
#define NAND 25         //GP I/O 25 is the memory write detection input (pulse low) 

void setup() {
 //except for the UART TX, every I/O pin is an input 
 pinMode(NAND,INPUT_PULLUP); //NAND input not(E & Q & not(R/W))

 Serial1.begin(3000000); //UART TX on pin GP0

 pinMode(2,INPUT_PULLUP); //D0
 pinMode(3,INPUT_PULLUP);
 pinMode(4,INPUT_PULLUP);
 pinMode(5,INPUT_PULLUP);
 pinMode(6,INPUT_PULLUP);
 pinMode(7,INPUT_PULLUP);
 pinMode(8,INPUT_PULLUP);
 pinMode(9,INPUT_PULLUP); //D7
 pinMode(10,INPUT_PULLUP); //A0
 pinMode(11,INPUT_PULLUP); 
 pinMode(12,INPUT_PULLUP); 
 pinMode(13,INPUT_PULLUP); 
 pinMode(14,INPUT_PULLUP); 
 pinMode(15,INPUT_PULLUP); 
 pinMode(16,INPUT_PULLUP); 
 pinMode(17,INPUT_PULLUP); //A7
 pinMode(18,INPUT_PULLUP); //A8
 pinMode(19,INPUT_PULLUP);
 pinMode(20,INPUT_PULLUP);
 pinMode(21,INPUT_PULLUP);
 pinMode(22,INPUT_PULLUP); //A12
 pinMode(26,INPUT_PULLUP); //A13
 pinMode(27,INPUT_PULLUP); //A14
 pinMode(28,INPUT_PULLUP); //A15
 pinMode(25,INPUT);   //UART RX is needed, remove LED and use GPIO25 as NAND input
 sam_reg[0xFFD2]=0;   //set the sam registers to default value
 sam_reg[0xFFD0]=0;
 sam_reg[0xFFCE]=0;
 sam_reg[0xFFCC]=0;
 sam_reg[0xFFCA]=0;
 sam_reg[0xFFC8]=1;
 sam_reg[0xFFC6]=0;  
 doffset=0x0400;      //text starts at 0x400 and graphics at 0x600
}

void setup1(){

}

void loop1() {

 while (gpio_get(NAND)==1); 
 while (gpio_get(NAND)==0); //wait for clock high to low to high
 
 x=gpio_get_all();       //get all 32 I/O data bits at once
 a = ((x>>10) & 0x1FFF) | ((x>>13) & 0xE000); //extract the 16-bit address
 if ((a>=0xFF20) && (a<0xFF40)) a = a & 0xFFE3;
 ccmem[a] = (x >> 2) & 0xFF;   //save the 8-bit data at the address

 if (a>=0x0FFC0) sam_reg[a & 0x00FE] = a & 1; // set/reset of sam registers

}

void loop(){

for (j=0; j<50;j++){ //50ms loop waiting for serial data from processor 2
// update address and modes using sam register S/R address bits  
   dmc = (sam_reg[0xC4]<<2) + (sam_reg[0xC2]<<1) + (sam_reg[0xC0]);
   doffset = (sam_reg[0xD2]<<6)+(sam_reg[0xD0]<<5)+(sam_reg[0xCE]<<4)+(sam_reg[0xCC]<<3)+(sam_reg[0xCA]<<2) + (sam_reg[0xC8]<<1) + sam_reg[0xC6];
   doffset = doffset * 0x200; 
   pia = ccmem[0xFF22];       //get the pia graphics mode bits

   if (Serial1.available() != 0) break; 
   delay(1);
 }

 if (Serial1.available() != 0) Serial1.read(); //clear one to two received bytes 
 if (Serial1.available() != 0) Serial1.read();
  
//send start record ff,00,aa,55,81
 Serial1.write(byte(0xFF));
 Serial1.write(byte(0x00));
 Serial1.write(byte(0xAA));
 Serial1.write(byte(0x55));
 Serial1.write(byte(0x81));

//get mode / memory size
 data_len = vid_size[dmc];
 data_len = 6144; //send all data, receiver can configure mode and size from gmode value
 gmode = 0;  //default to text

//set mode based on pia and dmc values
if ((pia & 0xF0) == 0x80) gmode = 8; 
if ((pia & 0xF0) == 0x90) gmode = 7; 
if ((pia & 0xF0) == 0xA0) gmode = 6; 
if ((pia & 0xF0) == 0xB0) gmode = 5; 
if ((pia & 0xF0) == 0xC0) gmode = 4; 
if ((pia & 0xF0) == 0xD0) gmode = 3; 
if ((pia & 0xF0) == 0xE0) gmode = 2; 
if ((pia & 0xF0) == 0xF0) gmode = 1; 
if (dmc == 0) gmode = 0;
if ((dmc == 0)&&((pia & 0x90)==0x10)) gmode = 9;
if ((dmc == 2)&&((pia & 0x90)==0x00)) gmode = 10;
if ((dmc == 4)&&((pia & 0x90)==0x00)) gmode = 11;
if ((dmc == 6)&&((pia & 0x90)==0x00)) gmode = 12;

 gmode = gmode | ((pia & 0x08)<<4); //get css bit
 Serial1.write(byte(gmode));
 
//transmit length
 Serial1.write(byte(data_len>>8));
 Serial1.write(byte(data_len & 0xFF)); 
 
//from disp_mem start to disp_mem end send data
 j=doffset;
 for (i=0;i<data_len;i++){
 Serial1.write(byte(ccmem[j++]));
 }

//send terminating bytes 00,55,ff,aa 
 Serial1.write(byte(0x00));
 Serial1.write(byte(0x55));
 Serial1.write(byte(0xFF));
 Serial1.write(byte(0xAA));


}

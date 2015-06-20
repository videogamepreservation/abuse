
#include <stdio.h>

char *uart_names[]={"none",
		    "8250 without scratch register",
		    "8250A or 16450",
		    "16C1450",
		    "16550 with defective FIFO",
		    "16550AF/C/CF",
		    "16C1550",
		    "16552 dual",
		    "82510"};

enum { UART_NONE,
       UART_8250,
       UART_8250A,
       UART_16C1450,
       UART_16550d,
       UART_16550AF,
       UART_16C1550,
       UART_16552,
       UART_82510 };
       



enum { 
  INTERRUPT_ID_REG=2,
  LINE_CONTROL_REG=3,
  MODEM_CONTROL_REG=4,
  SCRATCH_PAD_REG=7
};
  
void clear_interrupts();
#pragma aux clear_interrupts = "cli";

void enable_interrupts();
#pragma aux enable_interrupts = "sti";

void out_byte(int port, char value);
#pragma aux out_byte parm [dx] [al] = \
       "out dx, al", \
       "jmp short +2", \
       "jmp short +2";


char in_byte(int port);
#pragma aux in_byte parm [dx] = \
    "in al, dx",  \
    "jmp short +2", \
    "jmp short +2" \
    modify [al]; 



static void set_divisor_latch_bit1(short port)
{
  out_byte(port,in_byte(port+LINE_CONTROL_REG)&0x80);
}

static void set_divisor_latch_bit0(short port)
{
  out_byte(port,in_byte(port+LINE_CONTROL_REG)&0x7f);
}


// writes a byte to a port and reads it back returns read value
static char io_test(short port, char test_val)
{
  out_byte(port,test_val);
  return in_byte(port);
}

static int is_1655x(int port)
{
  set_divisor_latch_bit1(port);
  char bh=io_test(port+INTERRUPT_ID_REG,7);
  out_byte(port+INTERRUPT_ID_REG,0);     // reset register select
  if (bh!=7)
  {
    out_byte(port+MODEM_CONTROL_REG,0x80);     // turn power off
    char al=in_byte(port+MODEM_CONTROL_REG);   // check to see if bit was set
    out_byte(port+MODEM_CONTROL_REG,0);        // turn power back on
    if (al&0x80)
      return UART_16C1550;
    else return UART_16550AF;
  } 
  return UART_16552;
}

int detect_uart(short port)
{
  int type;
  clear_interrupts();
  set_divisor_latch_bit1(port);
  if (io_test(port,0x5a)!=0x5a || io_test(port,0xa5)!=0xa5)
    type=UART_NONE;
  {
    enable_interrupts();  
    if (io_test(port+SCRATCH_PAD_REG,0x5a)!=0x5a || 
	io_test(port+SCRATCH_PAD_REG,0xa5)!=0xa5)
      type=UART_8250;
    else
    {
      clear_interrupts();      
      if ((in_byte(port+INTERRUPT_ID_REG)&0xc0)==0xc0)  // FIFO not enabled, try to enable it
        type=is_1655x(port);
      else
      {
	out_byte(port+INTERRUPT_ID_REG,1);    // set to bank 0
	char bh=in_byte(port+INTERRUPT_ID_REG);
	out_byte(port+INTERRUPT_ID_REG,0);    // disable FIFO on 16550

	if ((bh&0xc0)==0xc0)                  // 1655x
	  type=is_1655x(port);
	else
	{
	  if ((bh&0xc0)==0x40)
	    type=UART_16550AF;
	  else if ((bh&0xc0)==0x80)
	    type=UART_16550d;
	  else
	  {
	    out_byte(port+INTERRUPT_ID_REG,0x60);    // set bank 3
	    char bh=in_byte(port+INTERRUPT_ID_REG);
	    out_byte(port+INTERRUPT_ID_REG,0);       // set bank 0
	    if ((bh&0x60)==0x60)
	      type=UART_82510;
	    else 
	    {
	      out_byte(port+MODEM_CONTROL_REG,0x80);    // see if power down is available
	      char bh=in_byte(port+MODEM_CONTROL_REG);
	      out_byte(port+MODEM_CONTROL_REG,0);    // power back on	      
	      if (bh&0x80)
	        type=UART_16C1450;
	      else type=UART_8250A;
	    }
	  }
	}		
      }
    }
  }

  enable_interrupts();  
  set_divisor_latch_bit1(port);
  return type;
}



main()
{
  for (int i=0;i<4;i++)
  {
    int port=*((unsigned short *)((0x40<<4)+i*2));
    char *name=(port==0 ? "None" : uart_names[detect_uart(port)]);
    printf("com%d : %x, %s\n",i,port,name);
  }
}

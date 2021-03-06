/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */



#include "gpio.h"
#include "libc.h"
#include "miniuart.h"
#include <stdarg.h>

#define AUX_IRQ_REG     ((volatile unsigned int*)(MMIO_BASE+0x00215000))
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

#define IER_REG_EN_REC_INT    (1 << 0)
#define IER_REG_INT           (3 << 2) // Must be set to receive interrupts
#define IER_REG_VALUE         (IER_REG_EN_REC_INT | IER_REG_INT)

#define IIR_REG_REC_NON_EMPTY (2 << 1)

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init(void){
    unsigned int reg;
    reg = *(GPFSEL1);
    reg &= ~(7<<12);
    reg |= 2<<12;
    reg &= ~(7<<15);
    reg |= 2<<15;
    *(GPFSEL1) = reg;

    *(GPPUD) = 0;
    reg=150; while(reg--) { asm volatile("nop"); }
    *(GPPUDCLK0) = (1<<14) | (1<<15);
    reg=150; while(reg--) { asm volatile("nop"); }
    *(GPPUDCLK0) = 0;
    *(AUX_ENABLE) = 1;
    *(AUX_MU_CNTL) = 0;

    *(AUX_MU_IER) = IER_REG_VALUE;   //Enable receive interrupts


    *(AUX_MU_LCR) = 3;
    *(AUX_MU_MCR) = 0;
    *(AUX_MU_BAUD) = 270;
    *(AUX_MU_CNTL) = 3;
}
void uart_init2()
{
    register unsigned int r;

    /* initialize UART */
    /*
     * 1. Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
     * 2. Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
     * 3. Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
     * 4. Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
     * 5. Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
     * 6. Set AUX_MU_BAUD to 270. Set baud rate to 115200
     * 7. Set AUX_MU_IIR_REG to 6. No FIFO.
     * 8. Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
     */

    *AUX_ENABLE |=1;      // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;

    *AUX_MU_IER = IER_REG_VALUE;   // ** Enable receive interrupts **
    /* *AUX_MU_IER = 0; */

    *AUX_MU_LCR = 3;      // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_BAUD = 270;   // 115200 baud
    /* *AUX_MU_IIR = 0x6;    // disable interrupts */

    /* map UART1 to GPIO pins */
    /* mini UART: */
    /*   GPIO14: ALT5->TX0 */
    /*   GPIO15: ALT5->RX0 */
    /* GPIO 14, 15 can be both used for mini UART and PL011 UART.
     * However, mini UART should set ALT5 and PL011 UART should set ALT0.
     * You need to configure GPFSELn register to change alternate function.
     * Next, you need to configure pull up/down register to disable GPIO pull up/down.
     * It’s because these GPIO pins use alternate functions, not basic input-output.
     * Please refer to the description of GPPUD and GPPUDCLKn registers for a detailed setup.
     */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;

    *GPPUD = 0;            // enable pins 14 and 15
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx



}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    /* wait until we can send */
    /* by checking the transmitter empty field */
    while (1) {
      if (*AUX_MU_LSR & 0x20)
        break;
    }
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}


/* flush the current data in mini-UART */
void uart_flush() {
  /* The bit 0 is show that if the receive FIFO holds at least 1 symbol */
  /* so this function will eat all the data inside the FIFO */
  /* until there are not exist any symbol */
  while (*AUX_MU_LSR&0x01) {
    (*AUX_MU_IO);
  }
}

/**
 * Receive a character
 */
char uart_getc() {
    /* wait until something is in the buffer */
    /* by checking the reciever empty field */
    while (1) {
      if(*AUX_MU_LSR&0x01)
        break;
    }
    /* read it and return */
    return (char)(*AUX_MU_IO);
}

/**
 * Display a string
 */
void uart_puts(char *s) {
    while(*s) {
        uart_send(*s++);
    }
}

/* Buffer queue */

void buf_push(char c);
char buf_pop();
int  buf_is_full();
int  buf_is_empty();

static int front = -1, back = -1;
static int capacity = BUF_SIZE;
void buf_push(char c) {
  while (buf_is_full()) ;
  uart_buffer[++back] = c;
}

char buf_pop() {
  while (buf_is_empty()) ;
  front++;
  return uart_buffer[front];
}

int buf_is_full() {
  return ((back + 1)%capacity == front);
}

int buf_is_empty() {
  return (front == back);
}


char getc() {
  return buf_pop();
}

void uart_irq_handler() {
    /* char c = uart_getc(); */
    /* uart_println("mini uart interrupted, received: %c", c); */

    /* There may be more than one byte in the FIFO */
    while ((*AUX_MU_IIR & IIR_REG_REC_NON_EMPTY) == IIR_REG_REC_NON_EMPTY) {
        buf_push(uart_getc());
    }
}

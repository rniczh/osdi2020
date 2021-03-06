#include "miniuart.h"
#include "libc.h"
#include "mbox.h"
#include "timer.h"
#include "framebuffer.h"
#include "bh.h"
#include "buddy.h"
#include "slab.h"



void test_buddy() {
  struct buddy *bd = global_bd;
  Buddy.show(bd);
  unsigned long free_arr[4] = {0,};
  for (int i = 0; i < 4; ++i) {
    /* allocate 16 pages */
    struct Pair p = Buddy.alloc(bd, 16);
    uart_println("alloc [%x to %x] w/ pair {%d, %d}", p.lb, p.ub, bd_phy2n(p.lb), bd_phy2n(p.ub));
    free_arr[i] = bd_phy2n(p.lb);
    Buddy.show(bd);
  }

  for (int i = 0; i < 4; ++i) {
    Buddy.dealloc(bd, free_arr[i]);
  }
  Buddy.show(bd);

}

struct test_struct {
  int a;
  int b;
  int c[10];
};


void test_slab_example() {
  /* initalize the slab allocator */
  /* it will reegsiter some specific size alloactor for it */
  kalloc_init(global_bd);
  uart_println("init kalloc");

  /* try to allocate a sizeof(i32)  and i64 integer */
  int *i32 = kalloc(sizeof(int));
  *i32 = 123;
  unsigned long *u64 = kalloc(sizeof(unsigned long));
  *u64 = 456;

  uart_println("i32: %d , u64: %d", *i32, *u64);

  kfree(i32);
  kfree(u64);


  /* try to allocate a sizeof(strct member) */
  struct test_struct *a = kalloc(sizeof(struct test_struct));
  a->a = 10;
  a->b = 11;
  a->c[5] = 12;

  uart_println("a: %d , b: %d, c[5]: %d", (*a).a, (*a).b, (*a).c[5]);

  kfree(a);
}

int getcmd(char *buf, int nbuf);

#define SWITCH_CONTINUE(buf, str, func)                                        \
  {                                                                            \
    if (sstrcmp(str, buf) == 0) {                                              \
      func();                                                                  \
      continue;                                                                \
    }                                                                          \
  }

/* === The functionality of the shell === */
/* help: display the help message */
void help() {
  uart_puts("Shell Usage:\r\n"
            "  help       Display this information.\r\n"
            "  hello      Display \"Hello World!\".\r\n"
            "  timestamp  Display current timestamp.\r\n"
            "  getel      Display the current exception level.\r\n"
            "  exc        Issue svc #1.\r\n"
            "  irq        Core/Local timer interrupt.\r\n"
            "  coretime   Core timer interrupt.\r\n"
            "  localtime  Local timer interrupt.\r\n"
            "  reboot     Reboot.\r\n"
            "  bhmode     enable bottom half mode.\r\n"
            );
}

/* hello: display hello world */
void hello() { uart_puts("Hello World!\r\n"); }

/* timestamp: display the "system" timer */
void get_system_timer() {
  asm volatile("stp x8, x9, [sp, #-16]!");
  asm volatile("mov x8, #1");   /* 1 for print system time */
  asm volatile("svc     #0");
  asm volatile("ldp x8, x9, [sp], #16");
  /* syscall to gettimer */

  /* register unsigned long long f, t; */

  /* /\* get the current counter frequency *\/ */
  /* asm volatile("mrs %0, cntfrq_el0" : "=r"(f)); */
  /* /\* read the current counter *\/ */
  /* asm volatile("mrs %0, cntpct_el0" : "=r"(t)); */

  /* /\* time = timer counter / timer frequency *\/ */
  /* unsigned long long i_part = t / f; */
  /* unsigned long long f_part = t * 100000000 / f % 100000000; */

  /* uart_println("[%d.%d]", i_part, f_part); */
  /* return t / f; */
}

/* reboot: reset the power of the board */
void reset() {
#define MMIO_BASE 0x3F000000
#define PM_RSTC ((volatile unsigned int *)(MMIO_BASE + 0x0010001c))
#define PM_RSTS ((volatile unsigned int *)(MMIO_BASE + 0x00100020))
#define PM_WDOG ((volatile unsigned int *)(MMIO_BASE + 0x00100024))
#define PM_WDOG_MAGIC 0x5a000000
#define PM_RSTC_FULLRST 0x00000020
  unsigned int r;
  // trigger a restart by instructing the GPU to boot from partition 0
  r = *PM_RSTS;
  r &= ~0xfffffaaa;
  *PM_RSTS = PM_WDOG_MAGIC | r; // boot from partition 0
  *PM_WDOG = PM_WDOG_MAGIC | 10;
  *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;

  while (1)
    ;
}

/* getel: get the current exception level */
void get_current_el() {
  int el;
  asm volatile("mrs %0, CurrentEL\n"
               "lsr %0, %0, #2" : "=r"(el));
  uart_println("Current exception level: %d", el);
}

/* exc: issue the svc #1 */
void svc1() { asm volatile("svc #1"); }

/* irq: local/core timer interrupt */
void timer_interrupt() {
  local_timer_init();
  core_timer_enable();
}

void core_timer_interrupt() {
  core_timer_enable();
}

void local_timer_interrupt() {
  local_timer_init();
}


void bottom_half_mode() {
  bh_mod_mask = 1;
  core_timer_enable_w4sec();
}

/* get the information of the cpu */
void get_board_revision() {
  /* buffer size in bytes (length of message) */
  mbox[0] = 7 * sizeof(unsigned int);

  mbox[1] = MBOX_REQUEST; /* Request message */

  /* ====== /Tags begin ====== */
  mbox[2] = MBOX_TAG_GETREV; /* get serial number command */
  mbox[3] = 4;               /* buffer size */
  mbox[4] = 0;
  mbox[5] = 0;
  /* the tag last for notify the mailbox */
  mbox[6] = MBOX_TAG_LAST;
  /* ====== Tags end/ ======== */

  /* send the message to the GPU and receive answer */
  if (mbox_call(MBOX_CH_PROP)) {
    uart_println("Revision number is: 0x%x", mbox[5]);
  } else {
    uart_println("Unable to qery revision!");
  }
}

void get_vc_memory() {
  /* buffer size in bytes (length of message) */
  mbox[0] = 8 * sizeof(unsigned int);

  /* Request message */
  mbox[1] = MBOX_REQUEST;

  /* ====== /Tags begin ====== */
  mbox[2] = MBOX_TAG_GETVCMEM;
  mbox[3] = 8;                 /* buffer size */
  mbox[4] = 0;
  /* 5-6 is reserve for output buffer */
  mbox[5] = 0; /* base address */
  mbox[6] = 0; /* size in bytes */
  /* the tag last for notify the mailbox */
  mbox[7] = MBOX_TAG_LAST;
  /* ====== Tags end/ ======== */

  /* send the message to the GPU and receive answer */
  if (mbox_call(MBOX_CH_PROP)) {
    uart_println("VC core base addr: 0x%x size 0x%x", mbox[5], mbox[6]);
  } else {
    uart_println("Unable to qery vc memory!");
  }
}

void get_arm_memory() {
  /* buffer size in bytes (length of message) */
  mbox[0] = 8 * sizeof(unsigned int);

  /* Request message */
  mbox[1] = MBOX_REQUEST;

  /* ====== /Tags begin ====== */
  mbox[2] = MBOX_TAG_GETARMMEM;
  mbox[3] = 8;                 /* buffer size */
  mbox[4] = 0;
  /* 5-6 is reserve for output buffer */
  mbox[5] = 0; /* base address */
  mbox[6] = 0; /* size in bytes */
  /* the tag last for notify the mailbox */
  mbox[7] = MBOX_TAG_LAST;
  /* ====== Tags end/ ======== */

  /* send the message to the GPU and receive answer */
  if (mbox_call(MBOX_CH_PROP)) {
    uart_println("ARM base addr: 0x%x size 0x%x", mbox[5], mbox[6]);
  } else {
    uart_println("Unable to qery ARM memory!");
  }
}

void shell() {
  get_board_revision();
  get_arm_memory();
  get_vc_memory();

  static char buf[1024];

  while (1) {
    if (getcmd(buf, sizeof(buf)) == -1)
      continue;
    SWITCH_CONTINUE(buf, "help",      help);
    SWITCH_CONTINUE(buf, "buddy",     test_buddy);
    SWITCH_CONTINUE(buf, "slab",      test_slab_example);
    SWITCH_CONTINUE(buf, "hello",     hello);
    SWITCH_CONTINUE(buf, "timestamp", get_system_timer);
    SWITCH_CONTINUE(buf, "show",      lfb_showpicture);
    SWITCH_CONTINUE(buf, "reboot",    reset);
    SWITCH_CONTINUE(buf, "getel",     get_current_el);
    SWITCH_CONTINUE(buf, "exc",       svc1);
    SWITCH_CONTINUE(buf, "irq",       timer_interrupt);
    SWITCH_CONTINUE(buf, "coretime",  core_timer_interrupt);
    SWITCH_CONTINUE(buf, "localtime", local_timer_interrupt);
    SWITCH_CONTINUE(buf, "bhmode",    bottom_half_mode);

    uart_println("[ERR] command `%s` not found", buf);
  }
}

int getcmd(char *buf, int nbuf) {
  uart_puts("# ");

  memset(buf, 0, nbuf);
  char *p_buf = buf;

  /* read from uart to buf until newline */
  char c;
  while ((c = getc()) != '\r') {
    if (c == 127 || c == 8) { /* backspace or delete */
      /* display */
      if (p_buf != buf) {
        /* display */
        uart_puts("\b \b");
        /* modified the buffer */
        *buf-- = 0;
      }
    } else {
      uart_send(c);
      *buf++ = c;
    }
  }
  uart_puts("\r\n");
  *buf = 0;

  return p_buf[0] == 0 ? -1 : 0;
}

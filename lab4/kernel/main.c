#include "bh.h"
#include "fork.h"
#include "framebuffer.h"
#include "irq.h"
#include "libc.h"
#include "miniuart.h"
#include "sched.h"
#include "shell.h"
#include "sys.h"
#include "timer.h"

int get_el() {
  int el;
  asm volatile("mrs %0, CurrentEL\n"
               "lsr %0, %0, #2"
               : "=r"(el));
  return el;
}

void user_process1(char *array) {

  char buf[2] = {0};
  while (1) {
    /* call_sys_write("User prcoess 1\r\n"); */
    for (int i = 0; i < 5; i++) {
      buf[0] = array[i];
      call_sys_write(buf);
      delay(100000);
    }
  }
}

void program() {
  call_sys_write("program started\r\n");
  while (1) {
    delay(100000);
  }
  call_sys_exit();
}

void init_process() {

  /* tty */
  int pid = call_sys_fork();
  if (pid == 0) {
    unsigned long stack = call_sys_malloc();
    if (stack < 0) {
      uart_println("Error while allocating stack for shell");
    } else {
      int err = call_sys_clone((unsigned long)&shell, 0, stack);
      if (err < 0) {
        uart_println("Error while clonning shell");
      }
    }
    call_sys_exit();
  }



  /* fork the first process (tty)
  /* TODO */
  /* int cnt = 1; */

  /* if (call_sys_fork() == 0) { */
  /*   call_sys_fork(); */
  /*   delay(10000000); */
  /*   call_sys_fork(); */
  /*   while(cnt < 10) { */
  /*     /\* uart_println("Task id: %d, cnt: %d\n", 123, cnt); *\/ */
  /*     delay(10000000); */
  /*     ++cnt; */
  /*   } */
  /*   call_sys_exit(); */
  /*   uart_println("shoud not be printed"); */
  /* } else { */
  /*   uart_println("Task %d before exec, cnt address 0x%x, cnt value %d\n",
   * 123, &cnt, cnt); */
  /* } */

  /* if (call_sys_fork() == 0) { */
  /*   uart_println("*[child] w/ task id: %d", call_sys_getid()); */
  /* } else { */
  /*   uart_println("*[parent] w/ task id: %d", call_sys_getid()); */
  /* } */

  /* if (call_sys_fork() == 0) { */
  /*   uart_println("[child] w/ task id: %d", call_sys_getid()); */
  /*   if (call_sys_fork() == 0) { */
  /*     uart_println("[child child] w/ task id: %d", call_sys_getid()); */
  /*   } else { */
  /*     uart_println("[child parent] w/ task id: %d", call_sys_getid()); */
  /*   } */
  /*   /\* get_el(); *\/ */
  /* } else { */
  /*   uart_println("[parent] w/ task id: %d", call_sys_getid()); */
  /*   /\* get_el(); *\/ */
  /* } */

  /* uart_println("id: %d", call_sys_getid()); */

  /* /\* char buf[30] = {0}; *\/ */
  /* /\* tfp_sprintf(buf, "User process started\n\r"); *\/ */
  /* /\* uart_println("Kernel process started. EL %d", get_el()); *\/ */
  /* call_sys_write("User prcoess started\r\n"); */

  /* /\* process 1 *\/ */
  /* unsigned long stack = call_sys_malloc(); */
  /* if (stack < 0) { */
  /*   uart_println("Error while allocating stack for process 1"); */
  /*   return; */
  /* } */
  /* uart_println("[User] Allocate a stack for user process @ %x", stack); */
  /* int err = call_sys_clone((unsigned long)&user_process1, */
  /*                          (unsigned long)"12345", stack); */
  /* if (err < 0) { */
  /*   uart_println("Error while clonning process 1"); */
  /*   return; */
  /* } */

  /* /\* process 2 *\/ */
  /* stack = call_sys_malloc(); */
  /* if (stack < 0) { */
  /*   uart_println("Error while allocating stack for process 2"); */
  /*   return; */
  /* } */

  /* uart_println("[User] Allocate a stack for user process @ %x", stack); */

  /* err = call_sys_clone((unsigned long)&user_process1, (unsigned long)"abcd",
   */
  /*                      stack); */
  /* if (err < 0) { */
  /*   uart_println("Error while clonning process 2"); */
  /*   return; */
  /* } */

  while (1) { delay(1000000); }
  call_sys_exit();
}

void kernel_process() {
  uart_println("Kernel process started. EL %d", get_el());
  do_exec(init_process);
}

void foo() {
  while (1) {
    /* uart_println("Task id: %d\n", current->pid); */
    delay(1000000);
  }
}

void idle() {
  while (1) {
    schedule();
  }
}

void el1_main() {
  uart_init();

  {
#include "mm.h"
    uart_println("Low memory: %x", LOW_MEMORY);
    uart_println("High memory: %x", HIGH_MEMORY);
  }

  /* local_timer_init(); */
  sys_core_timer_enable();

  int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
  if (res < 0) {
    uart_println("error while starting kernel process");
    return;
  }

  /* const int N = 10; */
  /* for (int i = 0; i < N; ++i) { // N should > 2 */

  /* } */

  idle();
}

int main(int argc, char *argv[]) {
  bh_mod_mask = 0;

  /* initialize uart with default clock & baud rate */
  uart_init();

  /* initialize framebuffer */
  lfb_init();

  /* start interactive shell */
  /* while (1); */
  shell();

  return 0;
}

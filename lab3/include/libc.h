#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

char *itoa(int val, int base);
int sstrcmp(const char *s1, const char *s2);
void *memset(void *s, int c, size_t n);
void memcpy(void *dest, const void *src, size_t num);
void uart_println(char *format, ...);
void uart_print(char *format, ...);
void  __attribute__((noreturn)) abort();

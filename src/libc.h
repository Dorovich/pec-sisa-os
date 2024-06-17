#pragma once
#ifndef LIBC_H
#define LIBC_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef char int8_t;
typedef int int16_t;
typedef unsigned int size_t;
typedef unsigned int uintptr_t;
typedef unsigned int syscall_value_t;

#define SYSCALL_FORK     0
#define SYSCALL_GETPID   1
#define SYSCALL_GETTICKS 2
#define SYSCALL_READKEY  3

void *memcpy(void *destination, const void *source, size_t num);
void *memset(void *ptr, int value, size_t num);

int __mulsi3(int a, int b);

static inline int syscall0_i(uint16_t number)
{
	__asm__(
		"and r0, %0, %0\n\t"
		"calls r0\n\t"
		: : "r"(number)
	);
}

static inline int syscall1_i(uint16_t number, uint16_t arg0)
{
	__asm__(
		"and r0, %0, %0\n\t"
		"and r1, %1, %1\n\t"
		"calls r0\n\t"
		: : "r"(number), "r"(arg0)
	);
}

#define fork()     ((int)syscall0_i(SYSCALL_FORK))
#define getpid()   ((int)syscall0_i(SYSCALL_GETPID))
#define getticks() ((unsigned int)syscall0_i(SYSCALL_GETTICKS))
#define readkey()  ((char)syscall0_i(SYSCALL_READKEY))

#endif

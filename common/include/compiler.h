#ifndef COMPILER_H
#define COMPILER_H

#define COMPILER_WEAK       __attribute__((weak))
#define COMPILER_USED       __attribute__((used))
#define COMPILER_NORETURN   __attribute__((noreturn))
#define COMPILER_ALIGNED(x) __attribute__((aligned(x)))
#define COMPILER_SECTION(x) __attribute__((section(x)))
#define COMPILER_NAKED      __attribute__((naked))

#endif

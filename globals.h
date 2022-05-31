#ifndef REPREPRO_GLOBALS_H
#define REPREPRO_GLOBALS_H

/* NOTE(ywen): I'm compiling on Ubuntu where <stdbool.h> is available. */
#include <stdbool.h>
#include <stddef.h>

#define xisspace(c) (isspace(c)!=0)
#define xisblank(c) (isblank(c)!=0)
#define xisdigit(c) (isdigit(c)!=0)

#define READONLY true
#define READWRITE false

#define ISSET(a, b) ((a & b) != 0)
#define NOTSET(a, b) ((a & b) == 0)

/* sometimes something is initializes though the value is never used to
 * work around some gcc uninitialized-use false-positives */
#define SETBUTNOTUSED(a) a

#ifdef SPLINT
#define UNUSED(a) /*@unused@*/ a
#define NORETURN
#define likely(a) (a)
#define unlikely(a) (a)
#else
#define likely(a) (!(__builtin_expect(!(a), false)))
#define unlikely(a) __builtin_expect(a, false)
#define NORETURN __attribute((noreturn))
#ifndef NOUNUSEDATTRIBUTE
#define UNUSED(a) a __attribute((unused))
#else
#define UNUSED(a) a
#endif
#endif

#define ARRAYCOUNT(a) (sizeof(a)/sizeof(a[0]))

#ifndef _D_EXACT_NAMLEN
#define _D_EXACT_NAMLEN(r) (strlen((r)->d_name))
#endif
/* for systems defining NULL to 0 instead of the nicer (void*)0 */
#define ENDOFARGUMENTS ((char *)0)

/* global information */
extern int verbose;

enum compression { c_none, c_gzip, c_bzip2, c_lzma, c_xz, c_lunzip, c_COUNT };

#define setzero(type, pointer) ({type *__var = pointer; memset(__var, 0, sizeof(type));})
#define NEW(type) ((type *)malloc(sizeof(type)))
#define nNEW(num, type) ((type *)malloc((num) * sizeof(type)))
#define zNEW(type) ((type *)calloc(1, sizeof(type)))
#define nzNEW(num, type) ((type *)calloc(num, sizeof(type)))
#define arrayinsert(type, array, position, length) ({type *__var = array; memmove(__var + (position) + 1, __var + (position), sizeof(type) * ((length) - (position)));})

#endif

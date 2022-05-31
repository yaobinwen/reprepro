#ifndef REPREPRO_MACROS_ATTRIBUTES_H
#define REPREPRO_MACROS_ATTRIBUTES_H

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

#endif

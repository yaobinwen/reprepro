#ifndef REPREPRO_INTERRUPT_H
#define REPREPRO_INTERRUPT_H

#include <stdbool.h>

extern volatile bool was_interrupted;
extern bool interruption_printed;

bool interrupted(void);

void g_fn_interrupt_signaled(int) /*__attribute__((signal))*/;

#endif

#include <stdio.h>
#include "interrupt.h"
#include "macros_attributes.h"

volatile bool was_interrupted = false;
bool interruption_printed = false;

bool interrupted(void) {
	if (was_interrupted) {
		if (!interruption_printed) {
			interruption_printed = true;
			fprintf(
				stderr,
				"\n\nInterruption in progress, interrupt again to force-stop it (and risking database corruption!)\n\n"
			);
		}
		return true;
	} else
		return false;
}

void g_fn_interrupt_signaled(UNUSED(int s)) {
	was_interrupted = true;
}

#ifndef REPREPRO_MACROS_CONFIG_H
#define REPREPRO_MACROS_CONFIG_H

#define CONFIGSET(variable, value) if (owner_ ## variable <= config_state) { \
					owner_ ## variable = config_state; \
					variable = value; }
#define CONFIGGSET(variable, value) if (owner_ ## variable <= config_state) { \
					owner_ ## variable = config_state; \
					global.variable = value; }
#define CONFIGDUP(variable, value) if (owner_ ## variable <= config_state) { \
					owner_ ## variable = config_state; \
					free(variable); \
					variable = strdup(value); \
					if (FAILEDTOALLOC(variable)) { \
						(void)fputs("Out of Memory!", \
								stderr); \
						exit(EXIT_FAILURE); \
					} }

#endif

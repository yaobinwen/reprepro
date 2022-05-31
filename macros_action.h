#ifndef REPREPRO_MACROS_ACTION_H
#define REPREPRO_MACROS_ACTION_H

#define y(type, name) type name
#define n(type, name) UNUSED(type dummy_ ## name)

#define ACTION_N(act, sp, args, name) static retvalue action_n_ ## act ## _ ## sp ## _ ## name ( \
			UNUSED(struct distribution *dummy2),         \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			int argc, args(const char *, argv[]))

#define ACTION_C(act, sp, name) static retvalue action_c_ ## act ## _ ## sp ## _ ## name ( \
			struct distribution *alldistributions,       \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			int argc, const char *argv[])

#define ACTION_B(act, sp, u, name) static retvalue action_b_ ## act ## _ ## sp ## _ ## name ( \
			u(struct distribution *, alldistributions),  \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			int argc, const char *argv[])

#define ACTION_L(act, sp, u, args, name) static retvalue action_l_ ## act ## _ ## sp ## _ ## name ( \
			struct distribution *alldistributions,       \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			int argc, args(const char *, argv[]))

#define ACTION_R(act, sp, d, a, name) static retvalue action_r_ ## act ## _ ## sp ## _ ## name ( \
			d(struct distribution *, alldistributions),  \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			a(int, argc), a(const char *, argv[]))

#define ACTION_T(act, sp, name) static retvalue action_t_ ## act ## _ ## sp ## _ ## name ( \
			UNUSED(struct distribution *ddummy),         \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			UNUSED(int argc), UNUSED(const char *dummy4[]))

#define ACTION_F(act, sp, d, a, name) static retvalue action_f_ ## act ## _ ## sp ## _ ## name ( \
			d(struct distribution *, alldistributions),  \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			a(int, argc), a(const char *, argv[]))

#define ACTION_RF(act, sp, ud, u, name) static retvalue action_rf_ ## act ## _ ## sp ## _ ## name ( \
			ud(struct distribution *, alldistributions),  \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			u(int, argc), u(const char *, argv[]))

#define ACTION_D(act, sp, u, name) static retvalue action_d_ ## act ## _ ## sp ## _ ## name ( \
			struct distribution *alldistributions,       \
			sp(const char *, section),                   \
			sp(const char *, priority),                  \
			act(const struct atomlist *, architectures), \
			act(const struct atomlist *, components),    \
			act(const struct atomlist *, packagetypes),  \
			u(int, argc), u(const char *, argv[]))

#endif

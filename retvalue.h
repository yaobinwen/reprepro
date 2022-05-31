#ifndef REPREPRO_RETVALUE_H
#define REPREPRO_RETVALUE_H

/* retvalue is simply an int.
 * just named to show it follows the given semantics */
/*@numabstract@*/ enum retvalue_enum {
	RET_ERROR_INCOMING_DENY = -13,
	RET_ERROR_INTERNAL = -12,
	RET_ERROR_BZ2 = -11,
	RET_ERROR_Z = -10,
	RET_ERROR_INTERRUPTED = -9,
	RET_ERROR_UNKNOWNFIELD = -8,
	RET_ERROR_MISSING = -7,
	RET_ERROR_BADSIG = -6,
	RET_ERROR_GPGME = -5,
	RET_ERROR_EXIST = -4,
	RET_ERROR_OOM = -3,
	RET_ERROR_WRONG_MD5 = -2,
	RET_ERROR = -1,
	RET_NOTHING = 0,
	RET_OK  = 1
};
typedef enum retvalue_enum retvalue;

#endif

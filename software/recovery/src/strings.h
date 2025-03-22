#ifndef _STRINGS_H_
#define _STRINGS_H_

#define DEFINE_STRING_LOCAL(name, value) static const char __far name[] = value
#ifdef STRINGS_H_IMPLEMENTATION
#define DEFINE_STRING(name, value) const char __far name[] = value
#else
#define DEFINE_STRING(name, value) extern const char __far name[]
#endif

DEFINE_STRING(s_nileswan_recovery, "nileswan recovery");

#endif /* _STRINGS_H_ */

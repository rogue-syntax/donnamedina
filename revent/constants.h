#ifndef __CONSTANTS__
#define __CONSTANTS__

#if SMTG_OS_WINDOWS && !defined(__GNUC__)
typedef long int32;
typedef unsigned long uint32;
#else
typedef int int32;
typedef unsigned int uint32;
#endif

#ifndef __PARTPARAMS__
#define __PARTPARAMS__
const int PARTPARAMS = 16;
#endif

#ifndef __PARTVOICES__
#define __PARTVOICES__
const int PARTVOICES = 121;
#endif

#ifndef __GENCOUNT__
#define __GENCOUNT__
const int GENCOUNT = 2;
#endif

class constants {
public:
	constants() {};
	~constants() {};
};
#endif


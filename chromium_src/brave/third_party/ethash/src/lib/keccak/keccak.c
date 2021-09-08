#include "build/build_config.h"

// The use of __builtin_cpu_init is wrapped around an __X86_64__ ifdef.
// Since this isn't available in vc++ and since it's only needed on macOS,
// we exclude this block with the following hack.
#if defined(OS_WIN)
#undef __x86_64__
#endif

#include "../../../../../../../../brave/third_party/ethash/src/lib/keccak/keccak.c"

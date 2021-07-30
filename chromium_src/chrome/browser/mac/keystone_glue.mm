#include "brave/browser/sparkle_buildflags.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/mac/sparkle_glue.h"
#endif

#define KeystoneEnabled KeystoneEnabled_ChromiumImpl
#define CurrentlyInstalledVersion CurrentlyInstalledVersion_ChromiumImpl
#include "../../../../../chrome/browser/mac/keystone_glue.mm"  // NOLINT
#undef KeystoneEnabled
#undef CurrentlyInstalledVersion

namespace keystone_glue {

bool KeystoneEnabled() {
#if BUILDFLAG(ENABLE_SPARKLE)
  return sparkle_glue::SparkleEnabled();
#else
  return false;
#endif
}

std::u16string CurrentlyInstalledVersion() {
#if BUILDFLAG(ENABLE_SPARKLE)
  return sparkle_glue::CurrentlyInstalledVersion();
#else
  return std::u16string();
#endif
}

}  // namespace keystone_glue

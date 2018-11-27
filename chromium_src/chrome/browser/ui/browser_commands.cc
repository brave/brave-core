#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_commands.h"
#define ReloadBypassingCache ReloadBypassingCache_ChromiumImpl
#include "../../../../../chrome/browser/ui/browser_commands.cc"
#undef ReloadBypassingCache

namespace chrome {

void ReloadBypassingCache(Browser* browser, WindowOpenDisposition disposition) {
  Profile* profile = browser->profile();
  DCHECK(profile);
  // NewTorIdentity will do hard reload after obtaining new identity
  if (profile->IsTorProfile())
    brave::NewTorIdentity(browser);
  else
    ReloadBypassingCache_ChromiumImpl(browser, disposition);
}

}  // namespace chrome

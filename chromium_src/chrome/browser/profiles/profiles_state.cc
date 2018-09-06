#include "chrome/browser/profiles/profiles_state.h"
#define IsRegularOrGuestSession IsRegularOrGuestSession_ChromiumImpl
#include "../../../../../../chrome/browser/profiles/profiles_state.cc"
#undef IsRegularOrGuestSession

namespace profiles {

bool IsRegularOrGuestSession(Browser* browser) {
  Profile* profile = browser->profile();
  if (profile->IsTorProfile())
    return true;
  return IsRegularOrGuestSession_ChromiumImpl(browser);
}

}  // namespace profiles

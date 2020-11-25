#include "brave/components/brave_sync/brave_sync_prefs.h"

void BraveRegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry) {
  brave_sync::Prefs::RegisterProfilePrefs(registry);
}

#define BRAVE_REGISTER_BROWSER_STATE_PREFS BraveRegisterBrowserStatePrefs(registry);
#include "../../../../../../ios/chrome/browser/prefs/browser_prefs.mm"

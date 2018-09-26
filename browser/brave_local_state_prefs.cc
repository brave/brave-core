/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"

#include "base/values.h"
#include "brave/browser/brave_stats_updater.h"
#include "brave/browser/referrals/brave_referrals_service.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  RegisterPrefsForBraveStatsUpdater(registry);
  RegisterPrefsForBraveReferralsService(registry);
#if defined(OS_MACOSX)
  // Turn off super annoying 'Hold to quit'
  registry->SetDefaultPrefValue(prefs::kConfirmToQuitEnabled,
      base::Value(false));
#endif
  tor::TorProfileService::RegisterLocalStatePrefs(registry);
}

}  // namespace brave

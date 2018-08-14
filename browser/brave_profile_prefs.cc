/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_profile_prefs.h"

#include "brave/browser/alternate_private_search_engine_util.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_sync/profile_prefs.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/sync/base/pref_names.h"

namespace brave {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  brave_shields::BraveShieldsWebContentsObserver::RegisterProfilePrefs(registry);
  brave_sync::prefs::BraveSyncPrefs::RegisterProfilePrefs(registry);

  RegisterAlternatePrivateSearchEngineProfilePrefs(registry);

  // appearance
  BraveThemeService::RegisterProfilePrefs(registry);
  registry->RegisterBooleanPref(kLocationBarIsWide, false);

  tor::TorProfileService::RegisterProfilePrefs(registry);

  registry->RegisterBooleanPref(kWidevineOptedIn, false);

  // Default Brave shields
  registry->RegisterBooleanPref(kHTTPSEVerywhereControlType, true);
  registry->RegisterBooleanPref(kNoScriptControlType, false);

  // No sign into Brave functionality
  registry->SetDefaultPrefValue(prefs::kSigninAllowed, base::Value(false));

  // Restore last profile on restart
  registry->SetDefaultPrefValue(prefs::kRestoreOnStartup,
      base::Value(SessionStartupPref::kPrefValueLast));

  // Show download prompt by default
  registry->SetDefaultPrefValue(prefs::kPromptForDownload, base::Value(true));

  // Not using chrome's web service for resolving navigation errors
  registry->SetDefaultPrefValue(prefs::kAlternateErrorPagesEnabled, base::Value(false));

  // Disable spell check service
  registry->SetDefaultPrefValue(spellcheck::prefs::kSpellCheckUseSpellingService, base::Value(false));

  // Disable safebrowsing reporting
  registry->SetDefaultPrefValue(prefs::kSafeBrowsingExtendedReportingOptInAllowed, base::Value(false));

  // Disable search suggestion
  registry->SetDefaultPrefValue(prefs::kSearchSuggestEnabled, base::Value(false));

  // Disable "Use a prediction service to load pages more quickly"
  registry->SetDefaultPrefValue(prefs::kNetworkPredictionOptions,
      base::Value(chrome_browser_net::NETWORK_PREDICTION_NEVER));

  // Make sync managed to dsiable some UI after password saving.
  registry->SetDefaultPrefValue(syncer::prefs::kSyncManaged, base::Value(true));
}

}  // namespace brave

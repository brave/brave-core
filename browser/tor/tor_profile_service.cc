/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/pref_registry/pref_registry_syncable.h"


namespace tor {

TorProfileService::TorProfileService() {
}

TorProfileService::~TorProfileService() {
}

// static
void TorProfileService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
   registry->RegisterBooleanPref(tor::prefs::kProfileUsingTor, false);
}
// static
void TorProfileService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
   registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                "socks5://127.0.0.1:9050");
}

void TorProfileService::AddObserver(TorLauncherServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TorProfileService::RemoveObserver(TorLauncherServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace tor

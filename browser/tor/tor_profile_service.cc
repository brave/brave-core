/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/version_info/channel.h"


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
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                   "socks5://127.0.0.1:9350");
      break;
    case version_info::Channel::BETA:
      registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                   "socks5://127.0.0.1:9360");
      break;
    case version_info::Channel::DEV:
      registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                   "socks5://127.0.0.1:9370");
      break;
    case version_info::Channel::CANARY:
      registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                   "socks5://127.0.0.1:9380");
      break;
    case version_info::Channel::UNKNOWN:
    default:
      registry->RegisterStringPref(tor::prefs::kTorProxyString,
                                   "socks5://127.0.0.1:9390");
  }
}

void TorProfileService::AddObserver(TorLauncherServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TorProfileService::RemoveObserver(TorLauncherServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace tor

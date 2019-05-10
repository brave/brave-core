/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include <string>

#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/version_info/channel.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

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

// static
std::string TorProfileService::CircuitIsolationKey(const GURL& url) {
  // https://2019.www.torproject.org/projects/torbrowser/design/#privacy
  //
  //    For the purposes of the unlinkability requirements of this
  //    section as well as the descriptions in the implementation
  //    section, a URL bar origin means at least the second-level DNS
  //    name.  For example, for mail.google.com, the origin would be
  //    google.com.  Implementations MAY, at their option, restrict
  //    the URL bar origin to be the entire fully qualified domain
  //    name.
  //
  // In particular, we need not isolate by the scheme,
  // username/password, port, path, or query part of the URL.
  url::Origin origin = url::Origin::Create(url);
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      origin.host(),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain.size() == 0)
    domain = origin.host();
  return domain;
}

void TorProfileService::AddObserver(TorLauncherServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TorProfileService::RemoveObserver(TorLauncherServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace tor

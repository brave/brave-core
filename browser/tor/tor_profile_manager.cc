/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_manager.h"

#include <algorithm>
#include <utility>

#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/translate/core/common/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION) || \
    BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#include "components/translate/core/browser/translate_pref_names.h"
#endif

namespace {
size_t GetTorBrowserCount() {
  BrowserList* list = BrowserList::GetInstance();
  return std::count_if(list->begin(), list->end(), [](Browser* browser) {
    return browser->profile()->IsTor();
  });
}
}  // namespace

// static
TorProfileManager& TorProfileManager::GetInstance() {
  static base::NoDestructor<TorProfileManager> instance;
  return *instance;
}

// static
void TorProfileManager::SwitchToTorProfile(
    Profile* original_profile,
    base::OnceCallback<void(Profile*)> callback) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(original_profile);
  profiles::OpenBrowserWindowForProfile(
      std::move(callback), /*always_create=*/false,
      /*is_new_profile=*/false, /*unblock_extensions=*/false, tor_profile);
}

// static
void TorProfileManager::CloseTorProfileWindows(Profile* tor_profile) {
  DCHECK(tor_profile);
  BrowserList::CloseAllBrowsersWithIncognitoProfile(
      tor_profile, base::DoNothing(), base::DoNothing(),
      true /* skip_beforeunload */);
}

TorProfileManager::TorProfileManager() {
  BrowserList::AddObserver(this);
}

TorProfileManager::~TorProfileManager() {
  BrowserList::RemoveObserver(this);
}

Profile* TorProfileManager::GetTorProfile(Profile* profile) {
  Profile* tor_profile = profile->GetOriginalProfile()->GetOffTheRecordProfile(
      Profile::OTRProfileID(tor::kTorProfileID),
      /*create_if_needed=*/true);

  const std::string context_id = tor_profile->UniqueId();
  auto it = tor_profiles_.find(context_id);
  if (it != tor_profiles_.end())
    return it->second;

  InitTorProfileUserPrefs(tor_profile);

  tor_profile->AddObserver(this);
  tor_profiles_[context_id] = tor_profile;

  tor::TorProfileService* service =
      TorProfileServiceFactory::GetForContext(tor_profile);
  DCHECK(service);
  // TorLauncherFactory relies on OnExecutableReady to launch tor process so we
  // need to make sure tor binary is there every time
  service->RegisterTorClientUpdater();

  return tor_profile;
}

void TorProfileManager::CloseAllTorWindows() {
  for (const auto& it : tor_profiles_)
    CloseTorProfileWindows(it.second);
}

void TorProfileManager::OnBrowserRemoved(Browser* browser) {
  if (!browser || !browser->profile()->IsTor())
    return;

  if (!GetTorBrowserCount()) {
    tor::TorProfileService* service =
        TorProfileServiceFactory::GetForContext(browser->profile());
    service->KillTor();
  }
}

void TorProfileManager::OnProfileWillBeDestroyed(Profile* profile) {
  const std::string context_id = profile->UniqueId();
  tor_profiles_.erase(context_id);
  profile->RemoveObserver(this);
}

void TorProfileManager::InitTorProfileUserPrefs(Profile* profile) {
  PrefService* pref_service = profile->GetPrefs();
  pref_service->SetString(prefs::kWebRTCIPHandlingPolicy,
                          blink::kWebRTCIPHandlingDisableNonProxiedUdp);
  pref_service->SetBoolean(prefs::kSafeBrowsingEnabled, false);
  // https://blog.torproject.org/bittorrent-over-tor-isnt-good-idea
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  pref_service->SetBoolean(kWebTorrentEnabled, false);
#endif
  // Disable the automatic translate bubble in Tor because we currently don't
  // support extensions in Tor mode and users cannot disable this through
  // settings page for Tor windows.
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION) || \
    BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  pref_service->SetBoolean(translate::prefs::kOfferTranslateEnabled, false);
#endif
}

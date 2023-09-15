/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_manager.h"

#include <algorithm>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"

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
Browser* TorProfileManager::SwitchToTorProfile(Profile* original_profile) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(original_profile);
  if (!tor_profile) {
    return nullptr;
  }

  // Find an existing Tor Browser, making a new one if no such Browser is
  // located.
  Browser* browser = chrome::FindTabbedBrowser(tor_profile, false);
  if (!browser && Browser::GetCreationStatusForProfile(tor_profile) ==
                      Browser::CreationStatus::kOk) {
    browser = Browser::Create(Browser::CreateParams(tor_profile, true));
    chrome::NewTab(browser);
    browser->window()->Show();
  }
  if (browser) {
    browser->window()->Activate();
  }
  return browser;
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
  if (TorProfileServiceFactory::IsTorDisabled(profile)) {
    return nullptr;
  }

  Profile* tor_profile = profile->GetOriginalProfile()->GetOffTheRecordProfile(
      Profile::OTRProfileID(tor::kTorProfileID),
      /*create_if_needed=*/true);

  const std::string context_id = tor_profile->UniqueId();
  auto it = tor_profiles_.find(context_id);
  if (it != tor_profiles_.end()) {
    return it->second;
  }

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
  for (const auto& it : tor_profiles_) {
    CloseTorProfileWindows(it.second);
  }
}

void TorProfileManager::OnBrowserRemoved(Browser* browser) {
  if (!browser || !browser->profile()->IsTor()) {
    return;
  }

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
  if (base::FeatureList::IsEnabled(net::features::kBraveTorWindowsHttpsOnly)) {
    pref_service->SetBoolean(prefs::kHttpsOnlyModeEnabled, true);
  }
  // https://blog.torproject.org/bittorrent-over-tor-isnt-good-idea
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  pref_service->SetBoolean(kWebTorrentEnabled, false);
#endif
  // Disable the automatic translate bubble in Tor because we currently don't
  // support extensions in Tor mode and users cannot disable this through
  // settings page for Tor windows.
  pref_service->SetBoolean(translate::prefs::kOfferTranslateEnabled, false);
}

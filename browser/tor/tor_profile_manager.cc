/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_manager.h"

#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"

// static
TorProfileManager& TorProfileManager::GetInstance() {
  static base::NoDestructor<TorProfileManager> instance;
  return *instance;
}

// static
void TorProfileManager::SwitchToTorProfile(
    Profile* original_profile,
    ProfileManager::CreateCallback callback) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(original_profile);
  profiles::OpenBrowserWindowForProfile(callback, false, false, false,
                                        tor_profile,
                                        Profile::CREATE_STATUS_INITIALIZED);
}

// static
void TorProfileManager::CloseTorProfileWindows(Profile* tor_profile) {
  if (tor_profile) {
    BrowserList::CloseAllBrowsersWithIncognitoProfile(
        tor_profile, base::DoNothing(), base::DoNothing(),
        true /* skip_beforeunload */);
  }
}

TorProfileManager::TorProfileManager() {
  BrowserList::AddObserver(this);
}

TorProfileManager::~TorProfileManager() {
  BrowserList::RemoveObserver(this);
}

Profile* TorProfileManager::GetTorProfile(Profile* original_profile) {
  Profile* tor_profile = original_profile->GetOffTheRecordProfile(
      Profile::OTRProfileID(tor::kTorProfileID));

  tor::TorProfileService* service =
      TorProfileServiceFactory::GetForContext(tor_profile);
  DCHECK(service);
  service->RegisterTorClientUpdater();
  // TODO(darkdh): pref init and extension restriction
  const std::string context_id = tor_profile->UniqueId();
  tor_profiles_[context_id] = tor_profile;
  return tor_profile;
}

void TorProfileManager::OnBrowserRemoved(Browser* browser) {
  // TODO(darkdh): make KillTor logic here and travere windows through
  // BrowserList
}

void TorProfileManager::OnProfileWillBeDestroyed(Profile* profile) {
  const std::string context_id = profile->UniqueId();
  auto it = tor_profiles_.find(context_id);
  if (it != tor_profiles_.end())
    tor_profiles_.erase(context_id);
}

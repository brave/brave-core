// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/profiles/profile_window.h"

#define CreateAndSwitchToNewProfile CreateAndSwitchToNewProfile_ChromiumImpl
#include "../../../../../chrome/browser/profiles/profile_window.cc"
#undef CreateAndSwitchToNewProfile

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"

namespace profiles {

void CreateAndSwitchToNewProfile(ProfileManager::CreateCallback callback,
                                 ProfileMetrics::ProfileAdd metric) {
  ProfileAttributesStorage& storage =
      g_browser_process->profile_manager()->GetProfileAttributesStorage();

  int avatar_index = storage.ChooseAvatarIconIndexForNewProfile();
  ProfileManager::CreateMultiProfileAsync(
      storage.ChooseNameForNewProfile(avatar_index),
      profiles::GetDefaultAvatarIconUrl(avatar_index),
      base::Bind(&profiles::OpenBrowserWindowForProfile, callback, true, true,
                 false));
  ProfileMetrics::LogProfileAddNewUser(metric);
}

void OpenBrowserWindowForTorProfile(ProfileManager::CreateCallback callback,
                                    bool always_create,
                                    bool is_new_profile,
                                    bool unblock_extensions,
                                    Profile* profile,
                                    Profile::CreateStatus status) {
  profiles::OpenBrowserWindowForProfile(
      callback, always_create, is_new_profile, unblock_extensions,
      profile->GetOffTheRecordProfile(), status);
  tor::TorProfileService* service =
      TorProfileServiceFactory::GetForProfile(profile);
  DCHECK(service);
  service->RegisterTorClientUpdater();
}

void OnTorRegularProfileCreated(ProfileManager::CreateCallback callback,
                                bool always_create,
                                bool is_new_profile,
                                bool unblock_extensions,
                                Profile* profile,
                                Profile::CreateStatus status) {
  // We need to postpone the timing of creating the off-the-record Tor profile
  // and let the regular Tor profile finish ProfileManager::DoFinalInit first.
  // So we pass the regular Tor profile here and access the off-the-record Tor
  // profile later when this task is executed.
  //
  // It is because an existing ProfileObserver might only start to observe
  // this regular profile in OnProfileAdded which is happened in DoFinalInit.
  // For example, ChromeProcessManagerDelegate has this behavior and we will
  // miss the OnOffTheRecordProfileCreated event if we create the
  // off-the-record Tor profile before OnProfileAdded of the regular Tor
  // profile is called. This would lead us to not destroying the background
  // hosts in time before the off-the-record Tor profile is destroyed and hit
  // DCHECK in ProfileDestroyer because a render process host wasn't destroyed
  // before the off-the-record profile is destroyed.
  DCHECK(base::SequencedTaskRunnerHandle::IsSet());
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&profiles::OpenBrowserWindowForTorProfile,
                                callback, always_create, is_new_profile,
                                unblock_extensions, profile, status));
}

#if !defined(OS_ANDROID)
void SwitchToTorProfile(ProfileManager::CreateCallback callback) {
  const base::FilePath& path = BraveProfileManager::GetTorProfilePath();
  // TODO(Anthony Tseng): profile metrics for tor
  // ProfileMetrics::LogProfileSwitch(ProfileMetrics::SWITCH_PROFILE_GUEST,
  //                                  g_browser_process->profile_manager(),
  //                                  path);
  g_browser_process->profile_manager()->CreateProfileAsync(
      path,
      base::Bind(&profiles::OnTorRegularProfileCreated, callback, false, false,
                 false),
      base::string16(), std::string());
}
#endif

void CloseTorProfileWindows() {
  // TODO(bridiver) - use GetLoadedProfiles and check IsTorProfile
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* profile = profile_manager->GetProfileByPath(
      BraveProfileManager::GetTorProfilePath());

  if (profile) {
    BrowserList::CloseAllBrowsersWithProfile(
        profile, base::Bind(&ProfileBrowserCloseSuccess),
        BrowserList::CloseCallback(), false);
  }
}

}  // namespace profiles

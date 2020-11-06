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

}  // namespace profiles

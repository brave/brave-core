// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

using BraveProfileWindowTest = InProcessBrowserTest;

namespace {

// An observer that returns back to test code after a new profile is
// initialized.
void OnUnblockOnProfileCreation(base::RunLoop* run_loop,
                                Profile* profile,
                                Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    run_loop->Quit();
}

}  // namespace

// Test that the browser command for creating new profiles
//  performs the random avatar lookup, and it gets applied to the profile.
IN_PROC_BROWSER_TEST_F(BraveProfileWindowTest,
    NewProfileGetsRandomNonPlaceholderAvatar) {
  // Create 2 additional profiles.
  base::RunLoop run_loop;
  profiles::CreateAndSwitchToNewProfile(
      base::Bind(&OnUnblockOnProfileCreation, &run_loop),
      ProfileMetrics::ADD_NEW_USER_MENU);
  run_loop.Run();
  base::RunLoop run_loop2;
  profiles::CreateAndSwitchToNewProfile(
      base::Bind(&OnUnblockOnProfileCreation, &run_loop2),
      ProfileMetrics::ADD_NEW_USER_MENU);
  run_loop2.Run();
  // Check new profiles have non-placeholder avatar and are all unique
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  std::vector<Profile*> all_profiles = ProfileManager::GetLastOpenedProfiles();
  size_t placeholder_avatar_index = profiles::GetPlaceholderAvatarIndex();
  std::vector<size_t> new_profile_avatars;
  for (Profile* profile : all_profiles) {
    if (profile->GetPath().BaseName() !=
        profile_manager->GetInitialProfileDir()) {
      size_t icon_index =
        profile->GetPrefs()->GetInteger(prefs::kProfileAvatarIndex);
      ASSERT_NE(icon_index, placeholder_avatar_index);
      // already seen?
      bool icon_duplicate_found = (std::find(
          new_profile_avatars.begin(),
          new_profile_avatars.end(),
          icon_index) != new_profile_avatars.end());
      ASSERT_NE(icon_duplicate_found, true);
      new_profile_avatars.push_back(icon_index);
    }
  }
}

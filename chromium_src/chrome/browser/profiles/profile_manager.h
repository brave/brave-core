/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

#define GetLastOpenedProfiles           \
  GetLastOpenedProfiles_ChromiumImpl(); \
  static std::vector<Profile*> GetLastOpenedProfiles
#define GetNumberOfProfiles virtual GetNumberOfProfiles
#define InitProfileUserPrefs virtual InitProfileUserPrefs
#define DoFinalInitForServices virtual DoFinalInitForServices
#define SetNonPersonalProfilePrefs virtual SetNonPersonalProfilePrefs
#define IsAllowedProfilePath virtual IsAllowedProfilePath
#define LoadProfileByPath virtual LoadProfileByPath
#define SetProfileAsLastUsed virtual SetProfileAsLastUsed
#define TestingProfileManager \
  TestingProfileManager;      \
  friend class BraveProfileManager

#include <chrome/browser/profiles/profile_manager.h>  // IWYU pragma: export
#undef TestingProfileManager
#undef SetProfileAsLastUsed
#undef LoadProfileByPath
#undef IsAllowedProfilePath
#undef SetNonPersonalProfilePrefs
#undef DoFinalInitForServices
#undef InitProfileUserPrefs
#undef GetNumberOfProfiles
#undef GetLastOpenedProfiles

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

#define InitProfileUserPrefs virtual InitProfileUserPrefs
#define DoFinalInitForServices virtual DoFinalInitForServices
#define SetNonPersonalProfilePrefs virtual SetNonPersonalProfilePrefs
#define IsAllowedProfilePath virtual IsAllowedProfilePath
#define LoadProfileByPath virtual LoadProfileByPath
#define TestingProfileManager \
  TestingProfileManager;      \
  friend class BraveProfileManager;

#include "src/chrome/browser/profiles/profile_manager.h"
#undef TestingProfileManager
#undef LoadProfileByPath
#undef IsAllowedProfilePath
#undef SetNonPersonalProfilePrefs
#undef DoFinalInitForServices
#undef InitProfileUserPrefs
#undef GetLastUsedProfileName

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

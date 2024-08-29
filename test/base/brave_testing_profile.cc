/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_testing_profile.h"

#include "brave/components/constants/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"

BraveTestingProfile::BraveTestingProfile(const base::FilePath& path,
                                         Delegate* delegate,
                                         CreateMode create_mode)
    : TestingProfile(path, delegate, create_mode) {}

BraveTestingProfile::BraveTestingProfile() : TestingProfile() {
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  GetPrefs()->SetBoolean(kBraveGCMChannelStatus, true);
#endif
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_testing_profile.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"

BraveTestingProfile::BraveTestingProfile(const base::FilePath& path,
                                         Delegate* delegate)
    : TestingProfile(path, delegate) {
  if (brave::IsSessionProfilePath(path)) {
    brave::CreateParentProfileData(this);
  }
}

BraveTestingProfile::BraveTestingProfile()
    : TestingProfile() {
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  GetPrefs()->SetBoolean(kBraveGCMChannelStatus, true);
#endif
}

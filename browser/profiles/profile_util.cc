/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"

#include "brave/browser/tor/buildflags.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace brave {

bool IsTorProfile(const Profile* profile) {
#if BUILDFLAG(ENABLE_TOR)
  DCHECK(profile);
  return profile->GetPrefs()->GetBoolean(tor::prefs::kProfileUsingTor);
#else
  return false;
#endif
}

bool IsGuestProfile(Profile* profile) {
  DCHECK(profile);
  return profile->GetOriginalProfile()->IsGuestSession() &&
         !IsTorProfile(profile);
}

}  // namespace brave

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

class Profile;
namespace {
void DestroyTorOrOffTheRecordProfile(Profile* const profile);
}

#include "../../../../../chrome/browser/profiles/profile_destroyer.cc"  // NOLINT

namespace {

void DestroyTorOrOffTheRecordProfile(Profile* const profile) {
  if (profile->IsTorProfile()) {
    profile->GetOriginalProfile()->DestroyTorProfile();
  } else {
    profile->GetOriginalProfile()->DestroyOffTheRecordProfile();
  }
}

}

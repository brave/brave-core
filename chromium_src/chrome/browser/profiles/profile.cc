/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/profiles/profile.h"

#include "brave/components/tor/tor_constants.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_utils.h"

#define BRAVE_ALLOWS_BROWSER_WINDOWS *this == TorID() ||

#define IsIncognitoProfile IsIncognitoProfile_ChromiumImpl
#define IsPrimaryOTRProfile IsPrimaryOTRProfile_ChromiumImpl
#include "src/chrome/browser/profiles/profile.cc"
#undef IsIncognitoProfile
#undef IsPrimaryOTRProfile
#undef BRAVE_ALLOWS_BROWSER_WINDOWS

namespace {
const char kSearchBackupResultsOTRProfileIDPrefix[] =
    "SearchBackupResults::OTR";
}  // namespace

// static
const Profile::OTRProfileID Profile::OTRProfileID::TorID() {
  return OTRProfileID(tor::kTorProfileID);
}

bool Profile::IsTor() const {
  return IsOffTheRecord() && GetOTRProfileID() == OTRProfileID::TorID();
}

bool Profile::IsIncognitoProfile() const {
  if (IsTor())
    return true;
  return IsIncognitoProfile_ChromiumImpl();
}

// Tor profile should behave like primary OTR profile used in private window
bool Profile::IsPrimaryOTRProfile() const {
  if (IsTor())
    return true;
  return IsPrimaryOTRProfile_ChromiumImpl();
}

Profile::OTRProfileID
Profile::OTRProfileID::CreateUniqueForSearchBackupResults() {
  return CreateUnique(kSearchBackupResultsOTRProfileIDPrefix);
}

bool Profile::OTRProfileID::IsSearchBackupResults() const {
  return base::StartsWith(profile_id_, kSearchBackupResultsOTRProfileIDPrefix,
                          base::CompareCase::SENSITIVE);
}

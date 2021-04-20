/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/profiles/profile.h"

#include "brave/components/tor/tor_constants.h"

#define BRAVE_ALLOWS_BROWSER_WINDOWS *this == TorID() ||

#define IsIncognitoProfile IsIncognitoProfile_ChromiumImpl
#include "../../../../../chrome/browser/profiles/profile.cc"
#undef IsIncognitoProfile

// static
const Profile::OTRProfileID Profile::OTRProfileID::TorID() {
  return OTRProfileID(tor::kTorProfileID);
}

// static
Profile::OTRProfileID Profile::OTRProfileID::CreateFromProfileID(
    const std::string& profile_id) {
  return OTRProfileID(profile_id);
}

bool Profile::IsTor() const {
  return IsOffTheRecord() && GetOTRProfileID() == OTRProfileID::TorID();
}

bool Profile::IsIncognitoProfile() const {
  if (IsTor())
    return true;
  return IsIncognitoProfile_ChromiumImpl();
}

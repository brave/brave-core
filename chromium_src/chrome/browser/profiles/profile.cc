/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_constants.h"

#define BRAVE_ALLOWS_BROWSER_WINDOWS *this == TorID() ||

#define BRAVE_IS_INCOGNITO_PROFILE                                    \
  if (IsOffTheRecord() && GetOTRProfileID() == OTRProfileID::TorID()) \
    return true;

#include "../../../../../chrome/browser/profiles/profile.cc"

// static
const Profile::OTRProfileID Profile::OTRProfileID::TorID() {
  return OTRProfileID(tor::kTorProfileID);
}

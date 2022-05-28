/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_io_data.h"
#include "brave/components/constants/url_constants.h"

#define IsHandledProtocol IsHandledProtocol_ChromiumImpl
#define IsHandledURL IsHandledURL_ChromiumImpl
#include "src/chrome/browser/profiles/profile_io_data.cc"
#undef IsHandledURL
#undef IsHandledProtocol

bool ProfileIOData::IsHandledProtocol(const std::string& scheme) {
  if (scheme == kBraveUIScheme)
    return true;
  if (scheme == "ipfs" || scheme == "ipns")
    return true;
  return IsHandledProtocol_ChromiumImpl(scheme);
}

// We need to provide our own version of IsHandledURL() as well to make sure
// that we get our own version of IsHandledProtocol() called when invoked via
// IsHandledURL(). Otherwise the old version will still be called since the
// renaming of IsHandledProtocol above does also modify the call point.
bool ProfileIOData::IsHandledURL(const GURL& url) {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }
  return IsHandledProtocol(url.scheme());
}

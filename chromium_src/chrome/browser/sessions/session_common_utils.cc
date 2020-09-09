/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/common/constants.h"
#include "url/gurl.h"

#define ShouldTrackURLForRestore ShouldTrackURLForRestore_ChromiumImpl

#include "../../../../../chrome/browser/sessions/session_common_utils.cc"

#undef ShouldTrackURLForRestore

bool ShouldTrackURLForRestore(const GURL& url) {
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host() == "wallet") {
    return false;
  } else if (url.SchemeIs("chrome-extension") &&
      url.host() == "odbfpeeihdkbihmopkbjmoonfanlbfcl") {
    return false;
  }
  return ShouldTrackURLForRestore_ChromiumImpl(url);
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "google_apis/google_api_keys.h"

namespace google_apis {
constexpr char kBraveAPIKeysDevelopersHowToURL[] =
    "https://community.brave.com";
}  // namespace google_apis

#define kAPIKeysDevelopersHowToURL kBraveAPIKeysDevelopersHowToURL
#include "src/chrome/browser/ui/startup/google_api_keys_infobar_delegate.cc"
#undef kAPIKeysDevelopersHowToURL

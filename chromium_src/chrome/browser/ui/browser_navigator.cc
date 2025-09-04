/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "chrome/browser/ui/browser_navigator_params.h"
#include "content/public/common/url_constants.h"
// Needed to prevent overriding url_typed_with_http_scheme
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "url/gurl.h"

namespace {

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

}  // namespace

// We want URLs that were manually typed with HTTP scheme to be HTTPS
// upgradable, but preserve the upstream's behavior in regards to captive
// portals (like hotel login pages which typically aren't cofnigured to work
// with HTTPS)
#define url_typed_with_http_scheme \
  url_typed_with_http_scheme;      \
  force_no_https_upgrade = false

#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL UpdateBraveScheme(params);
#include <chrome/browser/ui/browser_navigator.cc>
#undef BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL
#undef url_typed_with_http_scheme

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/webui_url_constants.h"
#include "url/gurl.h"

namespace {
// Forward delcare replacement function. The original file is patched to
// call this replacement.
bool BraveShouldThemifyFaviconForUrl(const GURL& url);
}  // namespace

#define ShouldThemifyFaviconForUrl ShouldThemifyFaviconForUrl_ChromiumImpl
#include "../../../../../../../chrome/browser/ui/views/tabs/tab_icon.cc"
#undef ShouldThemifyFaviconForUrl

namespace {
// Implementation of the replacement function checks for Brave-specific URLs for
// which the favicon should not be themified and then falls back onto the
// original Chromium implementation for all other URLs.
bool BraveShouldThemifyFaviconForUrl(const GURL& url) {
  if (url.SchemeIs(content::kChromeUIScheme) &&
      (url.host_piece() == kWelcomeHost ||
       url.host_piece() == kRewardsPageHost))
    return false;

  return ShouldThemifyFaviconForUrl_ChromiumImpl(url);
}
}  // namespace

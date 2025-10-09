// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/incognito_allowed_url.h"

#include <string>
#include <string_view>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace {

bool IsURLAllowedInIncognitoBraveImpl(const GURL& url) {
  if (url.scheme() != content::kChromeUIScheme) {
    return true;
  }

  std::string_view host = url.host_piece();
  if (host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kBraveUISyncHost || host == kAdblockHost ||
      host == kWelcomeHost || host == kBraveGettingStartedHost) {
    return false;
  }

  return true;
}

}  // namespace

#define BRAVE_IS_URL_ALLOWED_IN_INCOGNITO     \
  if (!IsURLAllowedInIncognitoBraveImpl(url)) \
    return false;

#include <chrome/browser/ui/incognito_allowed_url.cc>

#undef BRAVE_IS_URL_ALLOWED_IN_INCOGNITO

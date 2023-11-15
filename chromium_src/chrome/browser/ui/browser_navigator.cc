/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/common/webui_url_constants.h"

namespace {

bool IsHostAllowedInIncognitoBraveImpl(const std::string& scheme,
                                       const std::string_view host) {
  if (scheme != content::kChromeUIScheme) {
    return true;
  }

  // Brave specific rules.
  if (host == kWalletPageHost || host == kWalletPanelHost ||
      host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kChromeUISyncHost || host == kAdblockHost ||
      host == kWelcomeHost) {
    return false;
  }

  return true;
}

}  // namespace

// Need to replace brave scheme with chrome to go through upstream checks.
#define BRAVE_IS_HOST_ALLOWED_IN_INCOGNITO              \
  if (url.SchemeIs(content::kBraveUIScheme)) {          \
    scheme = content::kChromeUIScheme;                  \
  }                                                     \
  if (!IsHostAllowedInIncognitoBraveImpl(scheme, host)) \
    return false;
#include "src/chrome/browser/ui/browser_navigator.cc"
#undef BRAVE_IS_HOST_ALLOWED_IN_INCOGNITO

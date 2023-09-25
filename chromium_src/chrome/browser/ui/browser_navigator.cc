/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace {

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

bool IsHostAllowedInIncognitoBraveImpl(const std::string_view host) {
  if (host == kWalletPageHost || host == kWalletPanelHost ||
      host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kChromeUISyncHost || host == kAdblockHost ||
      host == kWelcomeHost) {
    return false;
  }

  return true;
}

}  // namespace

#define BRAVE_IS_HOST_ALLOWED_IN_INCOGNITO      \
  if (!IsHostAllowedInIncognitoBraveImpl(host)) \
    return false;
#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL UpdateBraveScheme(params);
#include "src/chrome/browser/ui/browser_navigator.cc"
#undef BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL
#undef BRAVE_IS_HOST_ALLOWED_IN_INCOGNITO

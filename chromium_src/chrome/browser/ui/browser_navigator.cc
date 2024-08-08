/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator_params.h"
// Needed to prevent overriding url_typed_with_http_scheme
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "url/gurl.h"

namespace {

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

bool IsURLAllowedInIncognitoBraveImpl(
    const GURL& url,
    content::BrowserContext* browser_context) {
  std::string scheme = url.scheme();
  std::string_view host = url.host_piece();
  if (scheme != content::kChromeUIScheme) {
    return true;
  }

  if (host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kChromeUISyncHost || host == kAdblockHost ||
      host == kWelcomeHost) {
    return false;
  }

  if (host == kWalletPageHost || host == kWalletPanelHost) {
    return browser_context &&
           user_prefs::UserPrefs::Get(browser_context)
               ->GetBoolean(kBraveWalletPrivateWindowsEnabled);
  }

  return true;
}

}  // namespace

// We want URLs that were manually typed with HTTP scheme to be HTTPS
// upgradable, but preserve the upstream's behavior in regards to captive
// portals (like hotel login pages which typically aren't cofnigured to work
// with HTTPS)
#define url_typed_with_http_scheme \
  url_typed_with_http_scheme;      \
  force_no_https_upgrade = false

#define BRAVE_IS_URL_ALLOWED_IN_INCOGNITO                      \
  if (!IsURLAllowedInIncognitoBraveImpl(url, browser_context)) \
    return false;
#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL UpdateBraveScheme(params);
#include "src/chrome/browser/ui/browser_navigator.cc"
#undef BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL
#undef BRAVE_IS_URL_ALLOWED_IN_INCOGNITO
#undef url_typed_with_http_scheme

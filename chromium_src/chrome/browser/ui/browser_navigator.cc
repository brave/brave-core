/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace {

constexpr char kBraveIsTor[] = "brave_is_tor";

std::pair<Browser*, int> BraveGetBrowserAndTabForDisposition(
    const NavigateParams& params);

void AdjustNavigateParamsForURLBraveImpl(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

bool IsHostAllowedInIncognitoBraveImpl(const base::StringPiece& host) {
  if (host == kRewardsHost ||
      host == kBraveUISyncHost ||
      host == chrome::kChromeUISyncInternalsHost) {
    return false;
  }

  return true;
}

}  // namespace

#define ChromeNavigationUIData BraveNavigationUIData
#include "../../../../chrome/browser/ui/browser_navigator.cc"  // NOLINT
#undef ChromeNavigationUIData

namespace {

std::pair<Browser*, int> BraveGetBrowserAndTabForDisposition(
    const NavigateParams& params) {
  Profile* profile = params.initiating_profile;

  // This temporary extra_headers will be reset to an empty string right after
  // this function is returned.
  if (params.extra_headers == kBraveIsTor) {
    return {GetOrCreateBrowser(profile->GetTorProfile(),
        params.user_gesture),
           -1};
  }

  return GetBrowserAndTabForDisposition(params);
}

}  // namespace

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace {

bool HandleURLInParent(NavigateParams* params, Profile* profile) {
  if (profile->IsTor() &&
      !params->browser->profile()->IsOffTheRecord()) {
    return true;
  }

  return false;
}

// GetOrCreateBrowser is not accessible here
Browser* BraveGetOrCreateBrowser(Profile* profile, bool user_gesture) {
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  return browser
             ? browser
             : Browser::Create(Browser::CreateParams(profile, user_gesture));
}

void UpdateBraveScheme(NavigateParams* params) {
  if (params->url.SchemeIs(content::kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    params->url = params->url.ReplaceComponents(replacements);
  }
}

void MaybeHandleInParent(NavigateParams* params, bool allow_in_incognito) {
  auto& profile = params->initiating_profile;
  if (brave::IsSessionProfile(profile)) {
    if (!allow_in_incognito) {
      params->initiating_profile =
          profile->IsOffTheRecord()
              ? brave::GetParentProfile(profile)->GetPrimaryOTRProfile(
                    /*create_if_needed=*/true)
              : brave::GetParentProfile(profile);
    } else if (HandleURLInParent(params, profile)) {
      params->browser = BraveGetOrCreateBrowser(
          brave::GetParentProfile(profile), params->user_gesture);
    }
  }
}

bool IsHostAllowedInIncognitoBraveImpl(const base::StringPiece& host) {
  if (host == kWalletPageHost || host == kWalletPanelHost ||
      host == kRewardsPageHost || host == chrome::kChromeUISyncInternalsHost ||
      host == chrome::kChromeUISyncHost) {
    return false;
  }

  return true;
}

}  // namespace

#define BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL           \
  UpdateBraveScheme(params);                           \
  MaybeHandleInParent(params, IsURLAllowedInIncognito( \
                                  params->url, params->initiating_profile));

#include "src/chrome/browser/ui/browser_navigator.cc"
#undef BRAVE_ADJUST_NAVIGATE_PARAMS_FOR_URL

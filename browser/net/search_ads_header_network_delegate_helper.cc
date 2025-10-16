/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/search_ads_header_network_delegate_helper.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "net/base/net_errors.h"

namespace {

// Returns true if the `Brave-Search-Ads` header should be set (to disable
// search ads), and false if it should not be set (to allow search ads).
bool ShouldSetHeaderForProfile(Profile* profile) {
  if (!profile || profile->IsOffTheRecord()) {
    return false;
  }
  const auto* prefs = profile->GetPrefs();

  if (!prefs->GetBoolean(brave_rewards::prefs::kEnabled)) {
    // If Rewards is disabled, show search ads.
    return false;
  }

  if (prefs->GetString(brave_rewards::prefs::kExternalWalletType).empty()) {
    // If Rewards is enabled but not connected, show search ads only if the user
    // has opted in.
    return !prefs->GetBoolean(brave_ads::prefs::kOptedInToSearchResultAds);
  }

  // If Rewards is enabled and connected, hide search ads.
  return true;
}

}  // namespace

namespace brave {

int OnBeforeStartTransaction_SearchAdsHeader(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> request) {
  // The header should be set if (to disable search ads):
  // - any of the following are true:
  //   - Rewards is enabled and not connected, and opted out of search ads.
  //   - Rewards is enabled and connected.
  // - and all of the following are true:
  //   - The current tab is not in a Private browser window.
  //   - `request_url` host is allowed.
  //   - `tab_origin` or `initiator_url` host is allowed.

  // The header should not be set if any one of the following are true. (to
  // allow search ads):
  // - The current tab is in a Private browser window.
  // - Rewards is disabled.
  // - Rewards is enabled and not connected, and opted-in to search ads.
  // - `request_url` host is disallowed.
  // - `tab_origin` and `initiator_url` hosts are disallowed.

  Profile* profile = Profile::FromBrowserContext(request->browser_context);
  if (ShouldSetHeaderForProfile(profile) &&
      brave_search::IsAllowedHost(request->request_url) &&
      (brave_search::IsAllowedHost(request->tab_origin) ||
       brave_search::IsAllowedHost(request->initiator_url))) {
    headers->SetHeader(kSearchAdsHeader, kSearchAdsDisabledValue);
    request->set_headers.insert(kSearchAdsHeader);
  }

  return net::OK;
}

}  // namespace brave

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/search_ads_header_network_delegate_helper.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace {

bool SearchAdsEnabledForProfile(Profile* profile) {
  if (!profile || profile->IsOffTheRecord()) {
    return true;
  }
  const auto* prefs = profile->GetPrefs();
  if (!prefs->GetBoolean(brave_rewards::prefs::kEnabled)) {
    return true;
  }
  return prefs->GetBoolean(brave_ads::prefs::kOptedInToSearchResultAds);
}

}  // namespace

namespace brave {

int OnBeforeStartTransaction_SearchAdsHeader(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  Profile* profile = Profile::FromBrowserContext(ctx->browser_context);

  // The Brave-Search-Ads header should be added with a negative value when all
  // of the following conditions are met:
  //   - The current tab is not in a Private browser window.
  //   - Brave Rewards is enabled for the profile.
  //   - The "Search Ads" option is not enabled for the profile.
  //   - The requested URL host is one of the Brave Search domains.
  //   - The request originates from one of the Brave Search domains.
  if (SearchAdsEnabledForProfile(profile) ||
      !brave_search::IsAllowedHost(ctx->request_url) ||
      (!brave_search::IsAllowedHost(ctx->tab_origin) &&
       !brave_search::IsAllowedHost(ctx->initiator_url))) {
    return net::OK;
  }

  headers->SetHeader(kSearchAdsHeader, kSearchAdsDisabledValue);
  ctx->set_headers.insert(kSearchAdsHeader);

  return net::OK;
}

}  // namespace brave

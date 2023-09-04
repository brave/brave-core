/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ads_status_header_network_delegate_helper.h"

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace brave {

int OnBeforeStartTransaction_AdsStatusHeader(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  Profile* profile = Profile::FromBrowserContext(ctx->browser_context);

  // The X-Brave-Ads-Enabled header should be added when Brave Private Ads are
  // enabled, the requested URL host is one of the Brave Search domains, and the
  // request originates from one of the Brave Search domains.
  if (!profile->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled) ||
      !brave_search::IsAllowedHost(ctx->request_url) ||
      (!brave_search::IsAllowedHost(ctx->tab_origin) &&
       !brave_search::IsAllowedHost(ctx->initiator_url))) {
    return net::OK;
  }

  headers->SetHeader(kAdsStatusHeader, kAdsEnabledStatusValue);
  ctx->set_headers.insert(kAdsStatusHeader);

  return net::OK;
}

}  // namespace brave

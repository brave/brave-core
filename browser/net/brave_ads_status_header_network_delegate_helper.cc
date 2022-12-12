/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ads_status_header_network_delegate_helper.h"

#include <string>
#include <vector>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

namespace brave {

int OnBeforeStartTransaction_AdsStatusHeader(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  brave_ads::AdsService* ads_service =
      brave_ads::AdsServiceFactory::GetForProfile(
          Profile::FromBrowserContext(ctx->browser_context));

  if (!ads_service || !ads_service->IsEnabled() ||
      !brave_search::IsAllowedHost(ctx->tab_origin) ||
      !brave_search::IsAllowedHost(ctx->request_url)) {
    return net::OK;
  }

  headers->SetHeader(kAdsStatusHeader, kAdsEnabledStatusValue);
  ctx->set_headers.insert(kAdsStatusHeader);

  return net::OK;
}

}  // namespace brave

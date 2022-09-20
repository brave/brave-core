/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_status_header_throttle.h"

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace {

constexpr char kAdsStatusHeader[] = "X-Brave-Ads-Enabled";
constexpr char kAdsEnabledStatusValue[] = "1";

}  // namespace

namespace brave_ads {

// static
std::unique_ptr<blink::URLLoaderThrottle>
AdsStatusHeaderThrottle::MaybeCreateThrottle(
    const AdsService* ads_service,
    const network::ResourceRequest& request) {
  DCHECK_EQ(request.resource_type,
            static_cast<int>(blink::mojom::ResourceType::kMainFrame));
  if (!ads_service || !ads_service->IsEnabled() ||
      !request.is_outermost_main_frame ||
      !brave_search::IsAllowedHost(request.url)) {
    return nullptr;
  }

  return std::make_unique<AdsStatusHeaderThrottle>();
}

AdsStatusHeaderThrottle::AdsStatusHeaderThrottle() = default;

AdsStatusHeaderThrottle::~AdsStatusHeaderThrottle() = default;

void AdsStatusHeaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* /* defer */) {
  DCHECK(request);
  DCHECK(brave_search::IsAllowedHost(request->url));

  request->headers.SetHeader(kAdsStatusHeader, kAdsEnabledStatusValue);
}

}  // namespace brave_ads

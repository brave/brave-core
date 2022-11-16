/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STATUS_HEADER_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STATUS_HEADER_THROTTLE_H_

#include <memory>

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace brave_ads {

class AdsService;

class AdsStatusHeaderThrottle : public blink::URLLoaderThrottle {
 public:
  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottle(
      const AdsService* ads_service,
      const network::ResourceRequest& request);

  AdsStatusHeaderThrottle();
  ~AdsStatusHeaderThrottle() override;

  AdsStatusHeaderThrottle(const AdsStatusHeaderThrottle&) = delete;
  AdsStatusHeaderThrottle& operator=(const AdsStatusHeaderThrottle&) = delete;
  AdsStatusHeaderThrottle(AdsStatusHeaderThrottle&&) = delete;
  AdsStatusHeaderThrottle& operator=(AdsStatusHeaderThrottle&&) = delete;

  // Implements blink::URLLoaderThrottle:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_STATUS_HEADER_THROTTLE_H_

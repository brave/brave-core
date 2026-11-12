// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_shields {
class BraveShieldsSettingsService;
}  // namespace brave_shields

namespace query_filter {

// Strips tracking query parameters from navigations. This ensure a service
// always receives a clean URL. Subresource filtering stays in
// the network delegate. See
// https://github.com/brave/brave-browser/issues/23742 for more details on sw.
class QueryFilterThrottle : public blink::URLLoaderThrottle {
 public:
  ~QueryFilterThrottle() override;

  QueryFilterThrottle(const QueryFilterThrottle&) = delete;
  QueryFilterThrottle& operator=(const QueryFilterThrottle&) = delete;

  // Returns a throttle when query filtering is enabled for `browser_context`.
  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottle(
      content::BrowserContext* browser_context,
      brave_shields::BraveShieldsSettingsService* shields_settings_service);

  // blink::URLLoaderThrottle:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  explicit QueryFilterThrottle(
      brave_shields::BraveShieldsSettingsService* shields_settings_service);

  raw_ptr<brave_shields::BraveShieldsSettingsService>
      shields_settings_service_ = nullptr;
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_

#include <memory>

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class GURL;
class PrefService;

namespace brave_shields {
class BraveShieldsSettingsService;
}  // namespace brave_shields

namespace query_filter {

// Strips tracking query parameters from non-renderer side navigations. This
// also ensure a service worker always receives a clean URL. Subresource
// filtering stays in the network delegate. See
// https://github.com/brave/brave-browser/issues/23742 for more details.
// TODO(https://github.com/brave/brave-browser/issues/61817): Follow-up on this
// with a renderer side throttler.
class QueryFilterThrottle : public blink::URLLoaderThrottle {
 public:
  ~QueryFilterThrottle() override;

  QueryFilterThrottle(const QueryFilterThrottle&) = delete;
  QueryFilterThrottle& operator=(const QueryFilterThrottle&) = delete;

  // Returns a throttle when query filtering is enabled for a request to `url`.
  // Throttles are created per request, so the settings at request start apply
  // for the lifetime of the request.
  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottle(
      const PrefService* prefs,
      brave_shields::BraveShieldsSettingsService* shields_settings_service,
      const GURL& url);

  // blink::URLLoaderThrottle:
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  QueryFilterThrottle();
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_THROTTLE_H_

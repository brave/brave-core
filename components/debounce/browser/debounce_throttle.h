/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_THROTTLE_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

class HostContentSettingsMap;

namespace debounce {

class DebounceService;

class DebounceThrottle : public blink::URLLoaderThrottle {
 public:
  DebounceThrottle(DebounceService* debounce_service,
                   HostContentSettingsMap* host_content_settings_map);
  ~DebounceThrottle() override;

  DebounceThrottle(const DebounceThrottle&) = delete;
  DebounceThrottle& operator=(const DebounceThrottle&) = delete;

  static std::unique_ptr<DebounceThrottle> MaybeCreateThrottleFor(
      DebounceService* debounce_service,
      HostContentSettingsMap* host_content_settings_map);

  // Implements blink::URLLoaderThrottle.
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      std::vector<std::string>* to_be_removed_request_headers,
      net::HttpRequestHeaders* modified_request_headers,
      net::HttpRequestHeaders* modified_cors_exempt_request_headers) override;

 private:
  DebounceService* debounce_service_ = nullptr;                  // not owned
  HostContentSettingsMap* host_content_settings_map_ = nullptr;  // not owned
  base::WeakPtrFactory<DebounceThrottle> weak_factory_{this};
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_THROTTLE_H_

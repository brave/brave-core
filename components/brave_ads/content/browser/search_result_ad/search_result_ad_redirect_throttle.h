/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/sessions/core/session_id.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

class SearchResultAdService;

class SearchResultAdRedirectThrottle : public blink::URLLoaderThrottle {
 public:
  static std::unique_ptr<SearchResultAdRedirectThrottle> MaybeCreateThrottleFor(
      SearchResultAdService* search_result_ad_service,
      const network::ResourceRequest& request,
      content::WebContents* web_contents);

  SearchResultAdRedirectThrottle(
      SearchResultAdService* search_result_ad_service,
      std::string creative_instance_id,
      SessionID tab_id);
  ~SearchResultAdRedirectThrottle() override;

  SearchResultAdRedirectThrottle(const SearchResultAdRedirectThrottle&) =
      delete;
  SearchResultAdRedirectThrottle& operator=(
      const SearchResultAdRedirectThrottle&) = delete;

  // Implements blink::URLLoaderThrottle.
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  raw_ptr<SearchResultAdService> search_result_ad_service_ = nullptr;
  std::string creative_instance_id_;
  SessionID tab_id_;

  base::WeakPtrFactory<SearchResultAdRedirectThrottle> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_

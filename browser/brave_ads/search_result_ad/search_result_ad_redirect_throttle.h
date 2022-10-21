/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"

namespace brave_ads {

// Monitors search result ad clicked request and redirects it to the landing
// page if the ad clicked event should be processed by the ads library.
class SearchResultAdRedirectThrottle : public blink::URLLoaderThrottle {
 public:
  static std::unique_ptr<SearchResultAdRedirectThrottle> MaybeCreateThrottleFor(
      const network::ResourceRequest& request,
      const content::WebContents::Getter& wc_getter);

  explicit SearchResultAdRedirectThrottle(
      const content::WebContents::Getter& wc_getter);
  ~SearchResultAdRedirectThrottle() override;

  SearchResultAdRedirectThrottle(const SearchResultAdRedirectThrottle&) =
      delete;
  SearchResultAdRedirectThrottle& operator=(
      const SearchResultAdRedirectThrottle&) = delete;

  // Implements blink::URLLoaderThrottle.
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  content::WebContents::Getter wc_getter_;

  base::WeakPtrFactory<SearchResultAdRedirectThrottle> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_REDIRECT_THROTTLE_H_

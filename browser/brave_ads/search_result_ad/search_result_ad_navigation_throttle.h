/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace brave_ads {

// Monitors search result ad clicked request and redirects it to the landing
// page if the ad clicked event should be processed by the ads library.
class SearchResultAdNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<SearchResultAdNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* handle);

  ~SearchResultAdNavigationThrottle() override;

  SearchResultAdNavigationThrottle(const SearchResultAdNavigationThrottle&) =
      delete;
  SearchResultAdNavigationThrottle& operator=(
      const SearchResultAdNavigationThrottle&) = delete;

  // Implements content::NavigationThrottle.
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  explicit SearchResultAdNavigationThrottle(content::NavigationHandle* handle);

  absl::optional<GURL> GetSearchResultAdTargetUrl(
      content::WebContents* web_contents,
      const GURL& navigation_url) const;

  void LoadSearchResultAdTargetUrl(
      content::WebContents* web_contents,
      const GURL& search_result_ad_target_url) const;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_NAVIGATION_THROTTLE_H_

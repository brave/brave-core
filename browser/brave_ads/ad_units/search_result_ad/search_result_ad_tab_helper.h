/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class GURL;

namespace content {
class RenderFrameHost;
}  // namespace content

namespace brave_ads {

class AdsService;
class SearchResultAdHandler;

class SearchResultAdTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SearchResultAdTabHelper> {
 public:
  explicit SearchResultAdTabHelper(content::WebContents* web_contents);
  ~SearchResultAdTabHelper() override;

  SearchResultAdTabHelper(const SearchResultAdTabHelper&) = delete;
  SearchResultAdTabHelper& operator=(const SearchResultAdTabHelper&) = delete;

  bool ShouldHandleSearchResultAdEvents() const;

  static void MaybeCreateForWebContents(content::WebContents* web_contents);

  void MaybeRetrieveSearchResultAd(content::RenderFrameHost* render_frame_host,
                                   bool should_trigger_viewed_event);

  void MaybeTriggerSearchResultAdClickedEvent(const GURL& navigation_url);

  static void SetAdsServiceForTesting(AdsService* ads_service);

 private:
  friend class content::WebContentsUserData<SearchResultAdTabHelper>;

  AdsService* GetAdsService();

  // content::WebContentsObserver overrides
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void WebContentsDestroyed() override;

  void MaybeProcessSearchResultAdClickedEvent(
      content::NavigationHandle* navigation_handle);

  void OnRetrieveSearchResultAd(std::vector<std::string> placement_ids);

  void OnCheckForAdWithDataPlacementIdVisible(const std::string& placement_id,
                                              base::Value value);

  std::unique_ptr<SearchResultAdHandler> search_result_ad_handler_;

  base::WeakPtrFactory<SearchResultAdTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_TAB_HELPER_H_

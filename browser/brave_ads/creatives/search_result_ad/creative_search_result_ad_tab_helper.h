/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TAB_HELPER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class GURL;
class PrefService;

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace brave_ads {

class AdsService;
class CreativeSearchResultAdHandler;

// Handles search result ad events in a tab. Owned by BraveTabFeatures.
class CreativeSearchResultAdTabHelper final
    : public tabs::ContentsObservingTabFeature {
 public:
  DECLARE_USER_DATA(CreativeSearchResultAdTabHelper);

  static CreativeSearchResultAdTabHelper* From(tabs::TabInterface* tab);

  // Returns nullptr for off-the-record profiles.
  static std::unique_ptr<CreativeSearchResultAdTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit CreativeSearchResultAdTabHelper(tabs::TabInterface& tab);

  CreativeSearchResultAdTabHelper(const CreativeSearchResultAdTabHelper&) =
      delete;
  CreativeSearchResultAdTabHelper& operator=(
      const CreativeSearchResultAdTabHelper&) = delete;

  ~CreativeSearchResultAdTabHelper() override;

  static void SetAdsServiceForTesting(AdsService* ads_service);

  void MaybeTriggerCreativeAdClickedEvent(const GURL& url,
                                          ResultCallback callback);

 private:
  AdsService* GetAdsService() const;

  PrefService* GetPrefs() const;

  void MaybeTriggerCreativeAdClickedEventCallback(bool success);

  void MaybeCreateCreativeSearchResultAdHandler(
      content::NavigationHandle* navigation_handle);

  void MaybeExtractCreativeAdPlacementIdsFromWebPageAndHandleViewedEvents();

  void MaybeHandleCreativeAdViewedEvents(
      std::vector<mojom::CreativeSearchResultAdInfoPtr>
          creative_search_result_ads);
  void MaybeHandleCreativeAdViewedEvent(
      mojom::CreativeSearchResultAdInfoPtr creative_search_result_ad);
  void MaybeHandleCreativeAdViewedEventCallback(
      mojom::CreativeSearchResultAdInfoPtr creative_search_result_ad,
      base::Value value);

  void MaybeHandleCreativeAdClickedEvent(const GURL& url);
  void MaybeHandleCreativeAdClickedEventCallback(
      mojom::CreativeSearchResultAdInfoPtr creative_search_result_ad);

  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  std::unique_ptr<CreativeSearchResultAdHandler>
      creative_search_result_ad_handler_;

  ui::ScopedUnownedUserData<CreativeSearchResultAdTabHelper>
      scoped_unowned_user_data_;

  base::WeakPtrFactory<CreativeSearchResultAdTabHelper> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_TAB_HELPER_H_

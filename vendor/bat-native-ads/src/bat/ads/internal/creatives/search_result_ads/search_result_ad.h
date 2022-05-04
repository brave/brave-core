/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_

#include "base/observer_list.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct SearchResultAdInfo;

class SearchResultAd final : public SearchResultAdObserver {
 public:
  SearchResultAd();
  ~SearchResultAd() override;

  void AddObserver(SearchResultAdObserver* observer);
  void RemoveObserver(SearchResultAdObserver* observer);

  void FireEvent(const mojom::SearchResultAdPtr& ad_mojom,
                 const mojom::SearchResultAdEventType event_type,
                 TriggerSearchResultAdEventCallback callback) const;

 private:
  void FireEvent(const SearchResultAdInfo& ad,
                 const mojom::SearchResultAdEventType event_type,
                 TriggerSearchResultAdEventCallback callback) const;
  void FireViewedEvent(const mojom::SearchResultAdPtr& ad_mojom,
                       TriggerSearchResultAdEventCallback callback) const;

  void NotifySearchResultAdEvent(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) const;
  void NotifySearchResultAdServed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdViewed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdClicked(const SearchResultAdInfo& ad) const;

  void NotifySearchResultAdEventFailed(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type,
      TriggerSearchResultAdEventCallback callback) const;

  base::ObserverList<SearchResultAdObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct SearchResultAdInfo;

class SearchResultAd final : public SearchResultAdObserver {
 public:
  SearchResultAd();
  ~SearchResultAd() override;

  void AddObserver(SearchResultAdObserver* observer);
  void RemoveObserver(SearchResultAdObserver* observer);

  void FireEvent(const std::string& uuid,
                 const std::string& creative_instance_id,
                 const mojom::SearchResultAdEventType event_type);

 private:
  base::ObserverList<SearchResultAdObserver> observers_;

  void FireEvent(const SearchResultAdInfo& ad,
                 const std::string& uuid,
                 const std::string& creative_instance_id,
                 const mojom::SearchResultAdEventType event_type);

  void NotifySearchResultAdEvent(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type) const;

  void NotifySearchResultAdServed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdViewed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdClicked(const SearchResultAdInfo& ad) const;

  void NotifySearchResultAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::SearchResultAdEventType event_type) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_H_

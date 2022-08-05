/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_AD_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

namespace search_result_ads {
class EventHandler;
}  // namespace search_result_ads

class Account;
class Transfer;
struct SearchResultAdInfo;

class SearchResultAd final : public search_result_ads::EventHandlerObserver {
 public:
  SearchResultAd(Account* account, Transfer* transfer);
  ~SearchResultAd() override;
  SearchResultAd(const SearchResultAd&) = delete;
  SearchResultAd& operator=(const SearchResultAd&) = delete;

  void TriggerEvent(mojom::SearchResultAdPtr ad_mojom,
                    const mojom::SearchResultAdEventType event_type,
                    TriggerSearchResultAdEventCallback callback);

 private:
  // search_result_ads::EventHandlerObserver:
  void OnSearchResultAdViewed(const SearchResultAdInfo& ad) override;
  void OnSearchResultAdClicked(const SearchResultAdInfo& ad) override;

  std::unique_ptr<search_result_ads::EventHandler> event_handler_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SEARCH_RESULT_AD_H_

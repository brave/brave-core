/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_CLICKED_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_CLICKED_H_

#include "bat/ads/internal/ad_events/ad_event_interface.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"

namespace ads {
namespace search_result_ads {

class AdEventClicked final : public AdEventInterface<SearchResultAdInfo> {
 public:
  AdEventClicked();
  ~AdEventClicked() override;
  AdEventClicked(const AdEventClicked&) = delete;
  AdEventClicked& operator=(const AdEventClicked&) = delete;

  void FireEvent(const SearchResultAdInfo& ad) override;
};

}  // namespace search_result_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_CLICKED_H_

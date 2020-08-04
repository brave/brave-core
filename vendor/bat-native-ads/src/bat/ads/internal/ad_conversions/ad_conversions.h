/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_CONVERSIONS_AD_CONVERSIONS_H_
#define BAT_ADS_INTERNAL_AD_CONVERSIONS_AD_CONVERSIONS_H_

#include <deque>
#include <string>

#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ad_conversions/ad_conversion_info.h"
#include "bat/ads/internal/ad_conversions/ad_conversion_queue_item_info.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class AdsImpl;

class AdConversions {
 public:
  AdConversions(
      AdsImpl* ads);

  ~AdConversions();

  void Initialize(
      InitializeCallback callback);

  void MaybeConvert(
      const std::string& url);

  void StartTimerIfReady();

 private:
  bool is_initialized_;
  InitializeCallback callback_;

  AdConversionQueueItemList queue_;

  Timer timer_;

  void OnGetAdConversions(
      const std::string& url,
      const Result result,
      const AdConversionList& ad_conversions);

  std::deque<AdHistory> FilterAdsHistory(
      const std::deque<AdHistory>& ads_history);
  std::deque<AdHistory> SortAdsHistory(
      const std::deque<AdHistory>& ads_history);

  AdConversionList FilterAdConversions(
      const std::string& url,
      const AdConversionList& ad_conversions);
  AdConversionList SortAdConversions(
      const AdConversionList& ad_conversions);

  void AddItemToQueue(
      const std::string& creative_instance_id,
      const std::string& creative_set_id);
  bool RemoveItemFromQueue(
      const std::string& creative_instance_id);
  void ProcessQueueItem(
      const AdConversionQueueItemInfo& info);
  void ProcessQueue();

  void StartTimer(
      const AdConversionQueueItemInfo& info);

  void Save();
  void OnSaved(
      const Result result);

  std::string ToJson();
  base::Value GetAsList();

  void Load();
  void OnLoaded(
      const Result result,
      const std::string& json);

  bool FromJson(
      const std::string& json);
  AdConversionQueueItemList GetFromList(
      const base::ListValue* list) const;
  bool GetFromDictionary(
      const base::DictionaryValue* dictionary,
      AdConversionQueueItemInfo* info) const;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_CONVERSIONS_AD_CONVERSIONS_H_

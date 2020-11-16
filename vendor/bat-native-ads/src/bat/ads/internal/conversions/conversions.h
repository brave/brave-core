/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
#define BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

#include <deque>
#include <string>

#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class AdsImpl;

class Conversions {
 public:
  Conversions(
      AdsImpl* ads);

  ~Conversions();

  void Initialize(
      InitializeCallback callback);

  bool ShouldAllow() const;

  void MaybeConvert(
      const std::string& url);

  void StartTimerIfReady();

 private:
  bool is_initialized_;
  InitializeCallback callback_;

  ConversionQueueItemList queue_;

  Timer timer_;

  void CheckUrl(
      const std::string& url);

  void Convert(
      const AdEventInfo& ad_event);

  ConversionList FilterConversions(
      const std::string& url,
      const ConversionList& conversions);
  ConversionList SortConversions(
      const ConversionList& conversions);

  void AddItemToQueue(
      const AdEventInfo& ad_event);
  bool RemoveItemFromQueue(
      const std::string& creative_instance_id);
  void ProcessQueueItem(
      const ConversionQueueItemInfo& queue_item);
  void ProcessQueue();

  void StartTimer(
      const ConversionQueueItemInfo& queue_item);

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
  ConversionQueueItemList GetFromList(
      const base::ListValue* list) const;
  bool GetFromDictionary(
      const base::DictionaryValue* dictionary,
      ConversionQueueItemInfo* info) const;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

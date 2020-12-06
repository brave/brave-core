/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
#define BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

#include <deque>
#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class Conversions {
 public:
  Conversions();

  ~Conversions();

  void AddObserver(
      ConversionsObserver* observer);
  void RemoveObserver(
      ConversionsObserver* observer);

  void Initialize(
      InitializeCallback callback);

  bool ShouldAllow() const;

  void MaybeConvert(
      const std::vector<std::string>& redirect_chain);

  void StartTimerIfReady();

 private:
  bool is_initialized_ = false;
  InitializeCallback callback_;

  base::ObserverList<ConversionsObserver> observers_;

  ConversionQueueItemList queue_;

  Timer timer_;

  void CheckRedirectChain(
      const std::vector<std::string>& redirect_chain);

  void Convert(
      const AdEventInfo& ad_event);

  ConversionList FilterConversions(
      const std::vector<std::string>& redirect_chain,
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

  void NotifyConversion(
      const std::string& creative_instance_id);

  void NotifyConversionFailed(
      const std::string& creative_instance_id);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

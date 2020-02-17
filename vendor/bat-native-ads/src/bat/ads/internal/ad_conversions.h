/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_CONVERSION_TRACKING_H_
#define BAT_ADS_INTERNAL_AD_CONVERSION_TRACKING_H_

#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/ad_conversion_queue_item_info.h"

#include "base/values.h"

namespace ads {

class AdsImpl;
class Client;

class AdConversions {
 public:
  AdConversions(
      AdsImpl* ads,
      AdsClient* ads_client,
      Client* client);
  ~AdConversions();

  void Initialize(
      InitializeCallback callback);

  void ProcessQueue();

  void AddToQueue(
      const std::string& creative_instance_id,
      const std::string& creative_set_id);

  bool OnTimer(
      const uint32_t timer_id);

 private:
  bool is_initialized_;
  InitializeCallback callback_;

  AdConversionQueueItemList queue_;

  uint32_t timer_id_;

  void ProcessQueueItem(
      const AdConversionQueueItemInfo& info);

  void StartTimer(
      const AdConversionQueueItemInfo& info);
  void StopTimer();

  bool Remove(
      const std::string& creative_instance_id);

  void SaveState();
  void OnStateSaved(
      const Result result);

  std::string ToJson();
  base::Value GetAsList();

  void LoadState();
  void OnStateLoaded(
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
  AdsClient* ads_client_;  // NOT OWNED
  Client* client_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_CONVERSION_TRACKING_H_

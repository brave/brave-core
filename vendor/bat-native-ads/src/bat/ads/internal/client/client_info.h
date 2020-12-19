/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_CLIENT_INFO_H_
#define BAT_ADS_INTERNAL_CLIENT_CLIENT_INFO_H_

#include <stdint.h>

#include <deque>
#include <map>
#include <string>

#include "bat/ads/ad_history_info.h"
#include "bat/ads/internal/ad_targeting/data_types/purchase_intent/purchase_intent_aliases.h"
#include "bat/ads/internal/ad_targeting/data_types/text_classification/text_classification_aliases.h"
#include "bat/ads/internal/client/preferences/ad_preferences_info.h"
#include "bat/ads/result.h"

namespace ads {

struct AdPreferencesInfo;

struct ClientInfo {
  ClientInfo();
  ClientInfo(
      const ClientInfo& state);
  ~ClientInfo();

  std::string ToJson();
  Result FromJson(
      const std::string& json);

  AdPreferencesInfo ad_preferences;
  std::deque<AdHistoryInfo> ads_shown_history;
  std::map<std::string, uint64_t> seen_ad_notifications;
  std::map<std::string, uint64_t> seen_advertisers;
  uint64_t next_ad_serving_interval_timestamp_ = 0;
  TextClassificationProbabilitiesList text_classification_probabilities;
  PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_CLIENT_INFO_H_

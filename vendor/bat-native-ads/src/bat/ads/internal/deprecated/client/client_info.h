/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

#include <map>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/deprecated/client/preferences/ad_preferences_info.h"
#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_aliases.h"
#include "bat/ads/internal/targeting/data_types/contextual/text_classification/text_classification_aliases.h"

namespace ads {

struct ClientInfo final {
  ClientInfo();
  ClientInfo(const ClientInfo& info);
  ~ClientInfo();

  std::string ToJson();
  bool FromJson(const std::string& json);

  AdPreferencesInfo ad_preferences;
  base::circular_deque<HistoryItemInfo> history;
  base::flat_map<std::string, std::map<std::string, bool>> seen_ads;
  base::flat_map<std::string, std::map<std::string, bool>> seen_advertisers;
  base::Time serve_ad_at;
  targeting::TextClassificationProbabilitiesList
      text_classification_probabilities;
  targeting::PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
  std::string version_code;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

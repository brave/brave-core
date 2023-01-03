/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/ads/serving/targeting/models/contextual/text_classification/text_classification_alias.h"
#include "bat/ads/internal/deprecated/client/preferences/ad_preferences_info.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace ads {

struct ClientInfo final {
  ClientInfo();

  ClientInfo(const ClientInfo& other);
  ClientInfo& operator=(const ClientInfo& other);

  ClientInfo(ClientInfo&& other) noexcept;
  ClientInfo& operator=(ClientInfo&& other) noexcept;

  ~ClientInfo();

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& root);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  AdPreferencesInfo ad_preferences;
  HistoryItemList history_items;
  base::flat_map<std::string, std::map<std::string, bool>> seen_ads;
  base::flat_map<std::string, std::map<std::string, bool>> seen_advertisers;
  targeting::TextClassificationProbabilityList
      text_classification_probabilities;
  targeting::PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

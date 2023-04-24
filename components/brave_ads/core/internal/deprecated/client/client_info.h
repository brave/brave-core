/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_alias.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/ad_preferences_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"

namespace brave_ads {

struct ClientInfo final {
  ClientInfo();

  ClientInfo(const ClientInfo&);
  ClientInfo& operator=(const ClientInfo&);

  ClientInfo(ClientInfo&&) noexcept;
  ClientInfo& operator=(ClientInfo&&) noexcept;

  ~ClientInfo();

  base::Value::Dict ToValue() const;
  bool FromValue(const base::Value::Dict& root);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  AdPreferencesInfo ad_preferences;
  HistoryItemList history_items;
  base::flat_map<std::string, std::map<std::string, bool>> seen_ads;
  base::flat_map<std::string, std::map<std::string, bool>> seen_advertisers;
  TextClassificationProbabilityList text_classification_probabilities;
  PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

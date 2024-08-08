/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/model/text_classification_alias.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

struct ClientInfo final {
  ClientInfo();

  ClientInfo(const ClientInfo&);
  ClientInfo& operator=(const ClientInfo&);

  ClientInfo(ClientInfo&&) noexcept;
  ClientInfo& operator=(ClientInfo&&) noexcept;

  ~ClientInfo();

  base::Value::Dict ToValue() const;
  [[nodiscard]] bool FromValue(const base::Value::Dict& dict);

  std::string ToJson() const;
  [[nodiscard]] bool FromJson(const std::string& json);

  AdHistoryList ad_history;
  TextClassificationProbabilityList text_classification_probabilities;
  PurchaseIntentSignalHistoryMap purchase_intent_signal_history;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_CLIENT_INFO_H_

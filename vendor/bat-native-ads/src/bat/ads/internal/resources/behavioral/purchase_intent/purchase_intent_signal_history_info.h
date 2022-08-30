/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"

namespace ads {
namespace targeting {

struct PurchaseIntentSignalHistoryInfo final {
  PurchaseIntentSignalHistoryInfo() = default;

  PurchaseIntentSignalHistoryInfo(base::Time created_at, uint16_t weight);

  PurchaseIntentSignalHistoryInfo(
      const PurchaseIntentSignalHistoryInfo& other) = default;
  PurchaseIntentSignalHistoryInfo& operator=(
      const PurchaseIntentSignalHistoryInfo& other) = default;

  PurchaseIntentSignalHistoryInfo(
      PurchaseIntentSignalHistoryInfo&& other) noexcept = default;
  PurchaseIntentSignalHistoryInfo& operator=(
      PurchaseIntentSignalHistoryInfo&&) noexcept = default;

  ~PurchaseIntentSignalHistoryInfo() = default;

  bool operator==(const PurchaseIntentSignalHistoryInfo& rhs) const;
  bool operator!=(const PurchaseIntentSignalHistoryInfo& rhs) const;

  base::Value::Dict ToValue() const;
  void FromValue(const base::Value::Dict& root);

  base::Time created_at;
  uint16_t weight = 0;
};

using PurchaseIntentSignalHistoryList =
    std::vector<PurchaseIntentSignalHistoryInfo>;
using PurchaseIntentSignalHistoryMap =
    std::map<std::string, PurchaseIntentSignalHistoryList>;

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SIGNAL_HISTORY_INFO_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT
#define BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT

#include <stdint.h>
#include <string>
#include <vector>
#include <deque>
#include <map>

#include "bat/ads/internal/search_providers.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_history.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_info.h"

namespace ads {
namespace classification {

using PurchaseIntentWinningCategoryList = std::vector<std::string>;

class PurchaseIntentClassifier {
 public:
  PurchaseIntentClassifier(
      const uint16_t signal_level,
      const uint16_t classification_threshold,
      const uint64_t signal_decay_time_window_in_seconds);

  ~PurchaseIntentClassifier();

  PurchaseIntentSignalInfo ExtractIntentSignal(
      const std::string& url);

  PurchaseIntentWinningCategoryList GetWinningCategories(
      const PurchaseIntentSignalSegmentHistoryMap& history,
      const uint16_t max_segments);

 private:
  uint16_t GetIntentScoreForHistory(
      const PurchaseIntentSignalSegmentHistoryList& segment_history);

  const uint16_t signal_level_;
  const uint16_t classification_threshold_;
  const uint64_t signal_decay_time_window_in_seconds_;
};

}  // namespace classification
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT

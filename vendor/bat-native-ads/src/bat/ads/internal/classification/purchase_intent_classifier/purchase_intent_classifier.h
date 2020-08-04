/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT
#define BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT

#include <stdint.h>

#include <string>
#include <vector>

#include "bat/ads/internal/classification/purchase_intent_classifier/funnel_keyword_info.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_history.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_signal_info.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/segment_keyword_info.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/site_info.h"
#include "bat/ads/internal/search_engine/search_providers.h"
#include "bat/ads/result.h"

namespace ads {

class AdsImpl;

namespace classification {

using PurchaseIntentWinningCategoryList = std::vector<std::string>;
using PurchaseIntentSegmentList = std::vector<std::string>;

class PurchaseIntentClassifier {
 public:
  explicit PurchaseIntentClassifier(
      AdsImpl* ads);

  ~PurchaseIntentClassifier();

  bool IsInitialized();

  bool Initialize(
      const std::string& json);

  void LoadUserModelForLocale(
      const std::string& locale);
  void LoadUserModelForId(
      const std::string& id);

  PurchaseIntentSignalInfo MaybeExtractIntentSignal(
      const std::string& url);

  PurchaseIntentWinningCategoryList GetWinningCategories(
      const PurchaseIntentSignalSegmentHistoryMap& history,
      const uint16_t max_segments);

 private:
  bool FromJson(
      const std::string& json);

  void OnLoadUserModelForId(
      const std::string& id,
      const Result result,
      const std::string& json);

  PurchaseIntentSignalInfo ExtractIntentSignal(
      const std::string& url);

  void AppendIntentSignalToHistory(
      const classification::PurchaseIntentSignalInfo& purchase_intent_signal);

  uint16_t GetIntentScoreForHistory(
      const PurchaseIntentSignalSegmentHistoryList& segment_history);

  SiteInfo GetSite(
      const std::string& url);

  PurchaseIntentSegmentList GetSegments(
      const std::string& search_query);

  uint16_t GetFunnelWeight(
      const std::string& search_query);

  std::vector<std::string> TransformIntoSetOfWords(
      const std::string& search_query);

  bool IsSubset(
      const std::vector<std::string>& set_a,
      const std::vector<std::string>& set_b);

  bool is_initialized_;
  uint16_t version_ = 0;
  uint16_t signal_level_ = 0;
  uint16_t classification_threshold_ = 0;
  uint64_t signal_decay_time_window_in_seconds_ = 0;
  std::vector<SiteInfo> sites_;  // sites2segments
  std::vector<SegmentKeywordInfo> segment_keywords_;  // keywords2segments
  std::vector<FunnelKeywordInfo> funnel_keywords_;  // keywords2funnelstages

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace classification
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLASSIFICATION_PURCHASE_INTENT_CLASSIFIER_PURCHASE_INTENT_CLASSIFIER_H_  // NOLINT

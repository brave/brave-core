/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_BEHAVIORAL_PURCHASE_INTENT_CLASSIFIER_SEGMENT_KEYWORD_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_BEHAVIORAL_PURCHASE_INTENT_CLASSIFIER_SEGMENT_KEYWORD_INFO_H_  // NOLINT

#include <string>
#include <vector>

namespace ads {
namespace ad_targeting {
namespace behavioral {

using PurchaseIntentSegmentList = std::vector<std::string>;

struct SegmentKeywordInfo {
 public:
  SegmentKeywordInfo();
  SegmentKeywordInfo(
      const PurchaseIntentSegmentList& segments,
      const std::string& keywords);
  SegmentKeywordInfo(
      const SegmentKeywordInfo& info);
  ~SegmentKeywordInfo();

  PurchaseIntentSegmentList segments;
  std::string keywords;
};

}  // namespace behavioral
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_BEHAVIORAL_PURCHASE_INTENT_CLASSIFIER_SEGMENT_KEYWORD_INFO_H_  // NOLINT

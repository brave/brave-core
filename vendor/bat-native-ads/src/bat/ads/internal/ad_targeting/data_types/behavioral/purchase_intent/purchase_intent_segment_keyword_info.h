/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_  // NOLINT

#include <string>

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

namespace ads {

struct PurchaseIntentSegmentKeywordInfo {
 public:
  PurchaseIntentSegmentKeywordInfo();
  PurchaseIntentSegmentKeywordInfo(
      const SegmentList& segments,
      const std::string& keywords);
  PurchaseIntentSegmentKeywordInfo(
      const PurchaseIntentSegmentKeywordInfo& info);
  ~PurchaseIntentSegmentKeywordInfo();

  SegmentList segments;
  std::string keywords;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_DATA_TYPES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_  // NOLINT

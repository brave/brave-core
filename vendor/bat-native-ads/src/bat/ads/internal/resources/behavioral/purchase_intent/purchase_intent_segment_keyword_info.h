/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_

#include <string>

#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace targeting {

struct PurchaseIntentSegmentKeywordInfo final {
 public:
  PurchaseIntentSegmentKeywordInfo();
  PurchaseIntentSegmentKeywordInfo(const SegmentList& segments,
                                   const std::string& keywords);
  PurchaseIntentSegmentKeywordInfo(
      const PurchaseIntentSegmentKeywordInfo& info);
  PurchaseIntentSegmentKeywordInfo& operator=(
      const PurchaseIntentSegmentKeywordInfo& info);
  ~PurchaseIntentSegmentKeywordInfo();

  SegmentList segments;
  std::string keywords;
};

}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_SEGMENT_KEYWORD_INFO_H_

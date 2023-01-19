/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_INFO_H_

#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::targeting {

struct UserModelInfo final {
  UserModelInfo();

  UserModelInfo(const UserModelInfo& other);
  UserModelInfo& operator=(const UserModelInfo& other);

  UserModelInfo(UserModelInfo&& other) noexcept;
  UserModelInfo& operator=(UserModelInfo&& other) noexcept;

  ~UserModelInfo();

  SegmentList interest_segments;
  SegmentList latent_interest_segments;
  SegmentList purchase_intent_segments;
};

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_TARGETING_USER_MODEL_INFO_H_

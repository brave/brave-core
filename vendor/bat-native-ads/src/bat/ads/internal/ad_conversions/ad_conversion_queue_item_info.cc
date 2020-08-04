/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_conversions/ad_conversion_queue_item_info.h"

namespace ads {

AdConversionQueueItemInfo::AdConversionQueueItemInfo() = default;

AdConversionQueueItemInfo::AdConversionQueueItemInfo(
    const AdConversionQueueItemInfo& info) = default;

AdConversionQueueItemInfo::~AdConversionQueueItemInfo() = default;

bool AdConversionQueueItemInfo::IsValid() const {
  if (creative_instance_id.empty() || creative_set_id.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads

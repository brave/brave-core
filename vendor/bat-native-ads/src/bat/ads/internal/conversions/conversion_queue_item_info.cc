/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversion_queue_item_info.h"

namespace ads {

ConversionQueueItemInfo::ConversionQueueItemInfo() = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    const ConversionQueueItemInfo& info) = default;

ConversionQueueItemInfo::~ConversionQueueItemInfo() = default;

bool ConversionQueueItemInfo::IsValid() const {
  if (creative_instance_id.empty() || creative_set_id.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads

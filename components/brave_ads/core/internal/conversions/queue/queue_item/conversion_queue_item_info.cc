/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"

#include <tuple>

namespace brave_ads {

ConversionQueueItemInfo::ConversionQueueItemInfo() = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    const ConversionQueueItemInfo& other) = default;

ConversionQueueItemInfo& ConversionQueueItemInfo::operator=(
    const ConversionQueueItemInfo& other) = default;

ConversionQueueItemInfo::ConversionQueueItemInfo(
    ConversionQueueItemInfo&& other) noexcept = default;

ConversionQueueItemInfo& ConversionQueueItemInfo::operator=(
    ConversionQueueItemInfo&& other) noexcept = default;

ConversionQueueItemInfo::~ConversionQueueItemInfo() = default;

bool ConversionQueueItemInfo::IsValid() const {
  return conversion.IsValid() && !process_at.is_null();
}

bool operator==(const ConversionQueueItemInfo& lhs,
                const ConversionQueueItemInfo& rhs) {
  const auto tie = [](const ConversionQueueItemInfo& conversion_queue_item) {
    return std::tie(conversion_queue_item.conversion,
                    conversion_queue_item.process_at,
                    conversion_queue_item.was_processed);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const ConversionQueueItemInfo& lhs,
                const ConversionQueueItemInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads

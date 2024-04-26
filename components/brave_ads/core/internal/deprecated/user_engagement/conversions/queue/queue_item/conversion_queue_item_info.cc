/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"

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
  return conversion.IsValid() && process_at;
}

}  // namespace brave_ads

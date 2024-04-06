/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_INFO_H_

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

struct ConversionQueueItemInfo final {
  ConversionQueueItemInfo();

  ConversionQueueItemInfo(const ConversionQueueItemInfo&);
  ConversionQueueItemInfo& operator=(const ConversionQueueItemInfo&);

  ConversionQueueItemInfo(ConversionQueueItemInfo&&) noexcept;
  ConversionQueueItemInfo& operator=(ConversionQueueItemInfo&&) noexcept;

  ~ConversionQueueItemInfo();

  bool operator==(const ConversionQueueItemInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  ConversionInfo conversion;
  base::Time process_at;
  bool was_processed = false;
};

using ConversionQueueItemList = std::vector<ConversionQueueItemInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_INFO_H_

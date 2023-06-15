/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"

namespace brave_ads {

struct ConversionQueueItemInfo final {
  ConversionQueueItemInfo();

  ConversionQueueItemInfo(const ConversionQueueItemInfo&);
  ConversionQueueItemInfo& operator=(const ConversionQueueItemInfo&);

  ConversionQueueItemInfo(ConversionQueueItemInfo&&) noexcept;
  ConversionQueueItemInfo& operator=(ConversionQueueItemInfo&&) noexcept;

  ~ConversionQueueItemInfo();

  bool operator==(const ConversionQueueItemInfo&) const;
  bool operator!=(const ConversionQueueItemInfo&) const;

  AdType ad_type = AdType::kUndefined;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  std::string conversion_id;
  std::string advertiser_public_key;
  base::Time process_at;
  bool was_processed = false;

  [[nodiscard]] bool IsValid() const;
};

using ConversionQueueItemList = std::vector<ConversionQueueItemInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_

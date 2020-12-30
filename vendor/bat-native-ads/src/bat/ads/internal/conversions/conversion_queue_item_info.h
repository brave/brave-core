/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_QUEUE_ITEM_INFO_H_
#define BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_QUEUE_ITEM_INFO_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace ads {

struct ConversionQueueItemInfo {
  ConversionQueueItemInfo();
  ConversionQueueItemInfo(
      const ConversionQueueItemInfo& info);
  ~ConversionQueueItemInfo();

  uint64_t timestamp_in_seconds = 0;
  std::string creative_instance_id;
  std::string creative_set_id;

  bool IsValid() const;
};

using ConversionQueueItemList = std::vector<ConversionQueueItemInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONVERSIONS_AD_CONVERSION_QUEUE_ITEM_INFO_H_

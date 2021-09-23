/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_

#include <string>

#include "base/time/time.h"

namespace ads {

struct ConversionQueueItemInfo final {
  ConversionQueueItemInfo();
  ConversionQueueItemInfo(const ConversionQueueItemInfo& info);
  ~ConversionQueueItemInfo();

  bool operator==(const ConversionQueueItemInfo& rhs) const;
  bool operator!=(const ConversionQueueItemInfo& rhs) const;

  std::string campaign_id;
  std::string creative_set_id;
  std::string creative_instance_id;
  std::string advertiser_id;
  std::string conversion_id;
  std::string advertiser_public_key;
  base::Time confirm_at;

  bool IsValid() const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_INFO_H_

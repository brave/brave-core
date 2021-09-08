/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

struct ConversionQueueItemInfo;

class ConversionsObserver : public base::CheckedObserver {
 public:
  // Invoked when an ad is converted
  virtual void OnConversion(
      const ConversionQueueItemInfo& conversion_queue_item) {}

  // Invoked when an ad fails to convert
  virtual void OnConversionFailed(
      const ConversionQueueItemInfo& conversion_queue_item) {}

 protected:
  ~ConversionsObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_OBSERVER_H_

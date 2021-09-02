/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_OBSERVER_H_

#include <cstdint>

#include "base/observer_list.h"
#include "base/time/time.h"

namespace ads {

struct AdInfo;

class AdTransferObserver : public base::CheckedObserver {
 public:
  // Invoked when an ad will be transferred
  virtual void OnWillTransferAd(const AdInfo& ad, const base::Time& time) {}

  // Invoked when an ad is transferred
  virtual void OnDidTransferAd(const AdInfo& ad) {}

  // Invoked when an ad transfer is cancelled
  virtual void OnCancelledAdTransfer(const AdInfo& ad, const int32_t tab_id) {}

  // Invoked when an ad fails to transfer
  virtual void OnFailedToTransferAd(const AdInfo& ad) {}

 protected:
  ~AdTransferObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_OBSERVER_H_

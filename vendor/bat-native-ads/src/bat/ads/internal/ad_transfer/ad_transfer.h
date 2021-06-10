/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_H_

#include <cstdint>
#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/ad_transfer/ad_transfer_observer.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class AdTransfer {
 public:
  AdTransfer();

  ~AdTransfer();

  void AddObserver(AdTransferObserver* observer);
  void RemoveObserver(AdTransferObserver* observer);

  void MaybeTransferAd(const int32_t tab_id, const std::string& url);

  void Cancel(const int32_t tab_id);

  void SetLastClickedAd(const AdInfo& ad);

 private:
  base::ObserverList<AdTransferObserver> observers_;

  int32_t transferring_ad_tab_id_ = 0;

  Timer timer_;

  AdInfo last_clicked_ad_;

  void clear_last_clicked_ad();

  void TransferAd(const int32_t tab_id, const std::string& url);

  void OnTransferAd(const int32_t tab_id, const std::string& url);

  void NotifyAdTransfer(const AdInfo& ad) const;

  void NotifyAdTransferFailed(const AdInfo& ad) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TRANSFER_AD_TRANSFER_H_

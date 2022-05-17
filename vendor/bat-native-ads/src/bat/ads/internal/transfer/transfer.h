/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_

#include <cstdint>
#include <vector>

#include "base/observer_list.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/base/timer.h"
#include "bat/ads/internal/tab_manager/tab_manager_observer.h"
#include "bat/ads/internal/transfer/transfer_observer.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace ads {

class Transfer final : public TabManagerObserver {
 public:
  Transfer();
  ~Transfer() override;

  void AddObserver(TransferObserver* observer);
  void RemoveObserver(TransferObserver* observer);

  void set_last_clicked_ad(const AdInfo& ad) { last_clicked_ad_ = ad; }

  void MaybeTransferAd(const int32_t tab_id,
                       const std::vector<GURL>& redirect_chain);

 private:
  void TransferAd(const int32_t tab_id,
                  const std::vector<GURL>& redirect_chain);
  void OnTransferAd(const int32_t tab_id,
                    const std::vector<GURL>& redirect_chain);

  void Cancel(const int32_t tab_id);

  void NotifyWillTransferAd(const AdInfo& ad, const base::Time time) const;
  void NotifyDidTransferAd(const AdInfo& ad) const;
  void NotifyCancelledTransfer(const AdInfo& ad, const int32_t tab_id) const;
  void NotifyFailedToTransferAd(const AdInfo& ad) const;

  // TabManagerObserver:
  void OnDidCloseTab(const int32_t id) override;

  base::ObserverList<TransferObserver> observers_;

  int32_t transferring_ad_tab_id_ = 0;

  Timer timer_;

  AdInfo last_clicked_ad_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_

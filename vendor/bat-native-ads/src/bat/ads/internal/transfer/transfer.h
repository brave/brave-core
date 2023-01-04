/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_

#include <cstdint>
#include <vector>

#include "base/observer_list.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/common/timer/timer.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"
#include "bat/ads/internal/transfer/transfer_observer.h"

namespace base {
class Time;
}  // namespace base

class GURL;

namespace ads {

struct TabInfo;

class Transfer final : public TabManagerObserver {
 public:
  Transfer();

  Transfer(const Transfer& other) = delete;
  Transfer& operator=(const Transfer& other) = delete;

  Transfer(Transfer&& other) noexcept = delete;
  Transfer& operator=(Transfer&& other) noexcept = delete;

  ~Transfer() override;

  void AddObserver(TransferObserver* observer);
  void RemoveObserver(TransferObserver* observer);

  void SetLastClickedAd(const AdInfo& ad) { last_clicked_ad_ = ad; }

  void MaybeTransferAd(int32_t tab_id, const std::vector<GURL>& redirect_chain);

 private:
  void TransferAd(int32_t tab_id, const std::vector<GURL>& redirect_chain);
  void OnTransferAd(int32_t tab_id, const std::vector<GURL>& redirect_chain);
  void OnLogAdEvent(const AdInfo& ad, bool success);

  void Cancel(int32_t tab_id);

  void FailedToTransferAd(const AdInfo& ad) const;

  void NotifyWillTransferAd(const AdInfo& ad, base::Time time) const;
  void NotifyDidTransferAd(const AdInfo& ad) const;
  void NotifyCanceledTransfer(const AdInfo& ad, int32_t tab_id) const;
  void NotifyFailedToTransferAd(const AdInfo& ad) const;

  // TabManagerObserver:
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidCloseTab(int32_t tab_id) override;

  base::ObserverList<TransferObserver> observers_;

  int32_t transferring_ad_tab_id_ = 0;

  Timer timer_;

  AdInfo last_clicked_ad_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TRANSFER_TRANSFER_H_

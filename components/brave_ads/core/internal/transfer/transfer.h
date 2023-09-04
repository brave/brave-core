/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_H_

#include <cstdint>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer_observer.h"
#include "brave/components/brave_ads/core/public/ad_info.h"

namespace base {
class Time;
}  // namespace base

class GURL;

namespace brave_ads {

struct TabInfo;

class Transfer final : public TabManagerObserver {
 public:
  Transfer();

  Transfer(const Transfer&) = delete;
  Transfer& operator=(const Transfer&) = delete;

  Transfer(Transfer&&) noexcept = delete;
  Transfer& operator=(Transfer&&) noexcept = delete;

  ~Transfer() override;

  void AddObserver(TransferObserver* observer);
  void RemoveObserver(TransferObserver* observer);

  void SetLastClickedAd(const AdInfo& ad) { last_clicked_ad_ = ad; }

  void MaybeTransferAd(int32_t tab_id, const std::vector<GURL>& redirect_chain);

 private:
  void TransferAd(int32_t tab_id, const std::vector<GURL>& redirect_chain);
  void TransferAdCallback(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain);
  void LogAdEventCallback(const AdInfo& ad, bool success);

  void Cancel(int32_t tab_id);

  void SuccessfullyTransferredAd(const AdInfo& ad) const;
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

  base::WeakPtrFactory<Transfer> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_H_

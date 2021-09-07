/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_transfer/ad_transfer.h"

#include "base/check.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/url_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {
const int64_t kTransferAdAfterSeconds = 10;
}  // namespace

AdTransfer::AdTransfer() = default;

AdTransfer::~AdTransfer() = default;

void AdTransfer::AddObserver(AdTransferObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdTransfer::RemoveObserver(AdTransferObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdTransfer::MaybeTransferAd(
    const int32_t tab_id,
    const std::vector<std::string>& redirect_chain) {
  if (!last_clicked_ad_.IsValid()) {
    return;
  }

  if (transferring_ad_tab_id_ == tab_id) {
    BLOG(1, "Already transferring ad for tab id " << tab_id);
    return;
  }

  if (!DomainOrHostExists(redirect_chain, last_clicked_ad_.target_url)) {
    BLOG(1, "Visited URL does not match the last clicked ad");
    return;
  }

  TransferAd(tab_id, redirect_chain);
}

void AdTransfer::Cancel(const int32_t tab_id) {
  if (transferring_ad_tab_id_ != tab_id) {
    return;
  }

  if (!timer_.Stop()) {
    return;
  }

  NotifyCancelledAdTransfer(last_clicked_ad_, tab_id);
}

///////////////////////////////////////////////////////////////////////////////

void AdTransfer::TransferAd(const int32_t tab_id,
                            const std::vector<std::string>& redirect_chain) {
  timer_.Stop();

  transferring_ad_tab_id_ = tab_id;

  const base::TimeDelta delay =
      base::TimeDelta::FromSeconds(kTransferAdAfterSeconds);

  const base::Time time = timer_.Start(
      delay, base::BindOnce(&AdTransfer::OnTransferAd, base::Unretained(this),
                            tab_id, redirect_chain));

  NotifyWillTransferAd(last_clicked_ad_, time);
}

void AdTransfer::OnTransferAd(const int32_t tab_id,
                              const std::vector<std::string>& redirect_chain) {
  const AdInfo ad = last_clicked_ad_;
  last_clicked_ad_ = {};

  transferring_ad_tab_id_ = 0;

  if (!TabManager::Get()->IsVisible(tab_id)) {
    NotifyFailedToTransferAd(ad);
    return;
  }

  const absl::optional<TabInfo> tab = TabManager::Get()->GetForId(tab_id);
  if (!tab) {
    NotifyFailedToTransferAd(ad);
    return;
  }

  if (!DomainOrHostExists(redirect_chain, tab->url)) {
    NotifyFailedToTransferAd(ad);
    return;
  }

  LogAdEvent(ad, ConfirmationType::kTransferred, [=](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log transferred ad event");
      NotifyFailedToTransferAd(ad);
      return;
    }

    BLOG(6, "Successfully logged transferred ad event");

    NotifyDidTransferAd(ad);
  });
}

void AdTransfer::NotifyWillTransferAd(const AdInfo& ad,
                                      const base::Time& time) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnWillTransferAd(ad, time);
  }
}

void AdTransfer::NotifyDidTransferAd(const AdInfo& ad) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnDidTransferAd(ad);
  }
}

void AdTransfer::NotifyCancelledAdTransfer(const AdInfo& ad,
                                           const int32_t tab_id) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnCancelledAdTransfer(ad, tab_id);
  }
}

void AdTransfer::NotifyFailedToTransferAd(const AdInfo& ad) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnFailedToTransferAd(ad);
  }
}

}  // namespace ads

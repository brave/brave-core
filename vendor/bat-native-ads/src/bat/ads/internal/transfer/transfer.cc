/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/transfer/transfer.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/time/time_formatting_util.h"
#include "bat/ads/internal/base/url/url_util.h"
#include "bat/ads/internal/tabs/tab_info.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace ads {

namespace {
constexpr base::TimeDelta kTransferAdAfter = base::Seconds(10);
}  // namespace

Transfer::Transfer() {
  TabManager::GetInstance()->AddObserver(this);
}

Transfer::~Transfer() {
  TabManager::GetInstance()->RemoveObserver(this);
}

void Transfer::AddObserver(TransferObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Transfer::RemoveObserver(TransferObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Transfer::MaybeTransferAd(const int32_t tab_id,
                               const std::vector<GURL>& redirect_chain) {
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

///////////////////////////////////////////////////////////////////////////////

void Transfer::TransferAd(const int32_t tab_id,
                          const std::vector<GURL>& redirect_chain) {
  timer_.Stop();

  transferring_ad_tab_id_ = tab_id;

  const base::Time transfer_ad_at = timer_.Start(
      FROM_HERE, kTransferAdAfter,
      base::BindOnce(&Transfer::OnTransferAd, base::Unretained(this), tab_id,
                     redirect_chain));

  BLOG(1, "Transfer ad for " << last_clicked_ad_.target_url << " "
                             << FriendlyDateAndTime(transfer_ad_at));

  NotifyWillTransferAd(last_clicked_ad_, transfer_ad_at);
}

void Transfer::OnTransferAd(const int32_t tab_id,
                            const std::vector<GURL>& redirect_chain) {
  const AdInfo ad = last_clicked_ad_;
  last_clicked_ad_ = {};

  transferring_ad_tab_id_ = 0;

  if (!TabManager::GetInstance()->IsTabVisible(tab_id)) {
    FailedToTransferAd(ad);
    return;
  }

  const absl::optional<TabInfo> tab =
      TabManager::GetInstance()->GetTabForId(tab_id);
  if (!tab) {
    FailedToTransferAd(ad);
    return;
  }

  if (!DomainOrHostExists(redirect_chain, tab->url)) {
    FailedToTransferAd(ad);
    return;
  }

  LogAdEvent(ad, ConfirmationType::kTransferred, [=](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log transferred ad event");
      FailedToTransferAd(ad);
      return;
    }

    BLOG(6, "Successfully logged transferred ad event");

    BLOG(1, "Transferred ad for " << ad.target_url);

    NotifyDidTransferAd(ad);
  });
}

void Transfer::Cancel(const int32_t tab_id) {
  if (transferring_ad_tab_id_ != tab_id) {
    return;
  }

  if (!timer_.Stop()) {
    return;
  }

  BLOG(1, "Cancelled ad transfer for creative instance id "
              << last_clicked_ad_.creative_instance_id << " with tab id "
              << tab_id);

  NotifyCancelledTransfer(last_clicked_ad_, tab_id);
}

void Transfer::FailedToTransferAd(const AdInfo& ad) const {
  BLOG(1, "Failed to transfer ad for " << ad.target_url);

  NotifyFailedToTransferAd(ad);
}

void Transfer::NotifyWillTransferAd(const AdInfo& ad,
                                    const base::Time time) const {
  for (TransferObserver& observer : observers_) {
    observer.OnWillTransferAd(ad, time);
  }
}

void Transfer::NotifyDidTransferAd(const AdInfo& ad) const {
  for (TransferObserver& observer : observers_) {
    observer.OnDidTransferAd(ad);
  }
}

void Transfer::NotifyCancelledTransfer(const AdInfo& ad,
                                       const int32_t tab_id) const {
  for (TransferObserver& observer : observers_) {
    observer.OnCancelledTransfer(ad, tab_id);
  }
}

void Transfer::NotifyFailedToTransferAd(const AdInfo& ad) const {
  for (TransferObserver& observer : observers_) {
    observer.OnFailedToTransferAd(ad);
  }
}

void Transfer::OnHtmlContentDidChange(const int32_t id,
                                      const std::vector<GURL>& redirect_chain,
                                      const std::string& content) {
  MaybeTransferAd(id, redirect_chain);
}

void Transfer::OnDidCloseTab(const int32_t id) {
  Cancel(id);
}

}  // namespace ads

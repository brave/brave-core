/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_transfer/ad_transfer.h"

#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/url_util.h"
#include "bat/ads/result.h"

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

void AdTransfer::MaybeTransferAd(const int32_t tab_id, const std::string& url) {
  if (!last_clicked_ad_.IsValid()) {
    return;
  }

  if (!DoesUrlHaveSchemeHTTPOrHTTPS(url)) {
    BLOG(1, "Visited URL is not supported for ad transfer");
    return;
  }

  if (!SameDomainOrHost(url, last_clicked_ad_.target_url)) {
    BLOG(1, "Visited URL does not match the last clicked ad");
    return;
  }

  if (transferring_ad_tab_id_ == tab_id) {
    BLOG(1, "Already transferring ad for tab id " << tab_id);
    return;
  }

  BLOG(1, "Visited URL matches the last clicked ad");

  TransferAd(tab_id, url);
}

void AdTransfer::SetLastClickedAd(const AdInfo& ad) {
  last_clicked_ad_ = ad;
}

void AdTransfer::Cancel(const int32_t tab_id) {
  if (transferring_ad_tab_id_ != tab_id) {
    return;
  }

  if (!timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Cancelling ad transfer for tab id " << tab_id);

  timer_.FireNow();
}

///////////////////////////////////////////////////////////////////////////////

void AdTransfer::clear_last_clicked_ad() {
  last_clicked_ad_ = AdInfo();
}

void AdTransfer::TransferAd(const int32_t tab_id, const std::string& url) {
  const base::TimeDelta delay =
      base::TimeDelta::FromSeconds(kTransferAdAfterSeconds);

  if (timer_.IsRunning()) {
    timer_.FireNow();
  }

  transferring_ad_tab_id_ = tab_id;

  const base::Time time =
      timer_.Start(delay, base::BindOnce(&AdTransfer::OnTransferAd,
                                         base::Unretained(this), tab_id, url));

  BLOG(1, "Transfer ad for " << url << " " << FriendlyDateAndTime(time));
}

void AdTransfer::OnTransferAd(const int32_t tab_id, const std::string& url) {
  const AdInfo ad = last_clicked_ad_;

  clear_last_clicked_ad();

  transferring_ad_tab_id_ = 0;

  if (!TabManager::Get()->IsVisible(tab_id)) {
    BLOG(1, "Failed to transfer ad for " << url);
    NotifyAdTransferFailed(ad);
    return;
  }

  const absl::optional<TabInfo> tab = TabManager::Get()->GetForId(tab_id);
  if (!tab) {
    BLOG(1, "Failed to transfer ad for " << url);
    NotifyAdTransferFailed(ad);
    return;
  }

  if (!SameDomainOrHost(tab->url, url)) {
    BLOG(1, "Failed to transfer ad for " << url);
    NotifyAdTransferFailed(ad);
    return;
  }

  BLOG(1, "Transferred ad for " << url);

  LogAdEvent(ad, ConfirmationType::kTransferred, [](const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log transferred ad event");
      return;
    }

    BLOG(6, "Successfully logged transferred ad event");
  });

  NotifyAdTransfer(ad);
}

void AdTransfer::NotifyAdTransfer(const AdInfo& ad) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnAdTransfer(ad);
  }
}

void AdTransfer::NotifyAdTransferFailed(const AdInfo& ad) const {
  for (AdTransferObserver& observer : observers_) {
    observer.OnAdTransferFailed(ad);
  }
}

}  // namespace ads

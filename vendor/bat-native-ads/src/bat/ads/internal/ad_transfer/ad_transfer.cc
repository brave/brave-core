/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_transfer/ad_transfer.h"

#include <stdint.h>

#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/tabs/tabs.h"
#include "bat/ads/internal/url_util.h"
#include "bat/ads/result.h"

namespace ads {

namespace {
const uint64_t kTransferAdAfterSeconds = 10;
}  // namespace

AdTransfer::AdTransfer(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdTransfer::~AdTransfer() = default;

void AdTransfer::MaybeTransferAd(
    const int32_t tab_id,
    const std::string& url) {
  if (!last_clicked_ad_.IsValid()) {
    return;
  }

  if (!UrlHasScheme(url)) {
    BLOG(1, "Visited URL is not supported for ad transfer");
    return;
  }

  if (!SameSite(url, last_clicked_ad_.target_url)) {
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

void AdTransfer::set_last_clicked_ad(
    const AdInfo& ad) {
  last_clicked_ad_ = ad;
}

void AdTransfer::Cancel(
    const int32_t tab_id) {
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

void AdTransfer::TransferAd(
    const int32_t tab_id,
    const std::string& url) {
  const base::TimeDelta delay =
      base::TimeDelta::FromSeconds(kTransferAdAfterSeconds);

  if (timer_.IsRunning()) {
    timer_.FireNow();
  }

  transferring_ad_tab_id_ = tab_id;

  const base::Time time = timer_.Start(
      delay, base::BindOnce(&AdTransfer::OnTransferAd,
          base::Unretained(this), tab_id, url));

  BLOG(1, "Transfer ad for " << url << " " << FriendlyDateAndTime(time));
}

void AdTransfer::OnTransferAd(
    const int32_t tab_id,
    const std::string& url) {
  const AdInfo transferred_ad = last_clicked_ad_;

  clear_last_clicked_ad();

  transferring_ad_tab_id_ = 0;

  if (!ads_->get_tabs()->IsVisible(tab_id)) {
    BLOG(1, "Failed to transfer ad for " << url);
    return;
  }

  BLOG(1, "Transferred ad for " << url);

  AdEvents ad_events(ads_);
  ad_events.Log(transferred_ad, ConfirmationType::kTransferred,
      [](const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log transferred ad event");
      return;
    }

    BLOG(6, "Successfully logged transferred ad event");
  });

  ads_->get_confirmations()->ConfirmAd(transferred_ad.creative_instance_id,
      ConfirmationType::kTransferred);
}

}  // namespace ads

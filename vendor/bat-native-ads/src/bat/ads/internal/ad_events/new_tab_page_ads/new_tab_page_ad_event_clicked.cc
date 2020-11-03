/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_clicked.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_transfer/ad_transfer.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {
namespace new_tab_page_ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kClicked;
}  // namespace

AdEventClicked::AdEventClicked(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventClicked::~AdEventClicked() = default;

void AdEventClicked::Trigger(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Clicked new tab page ad with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->get_ad_transfer()->set_last_clicked_ad(ad);

  AdEvents ad_events(ads_);
  ad_events.Log(ad, kConfirmationType, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log new tab page ad clicked event");
      return;
    }

    BLOG(6, "Successfully logged new tab page ad clicked event");
  });

  ads_->get_ads_history()->AddNewTabPageAd(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace new_tab_page_ads
}  // namespace ads

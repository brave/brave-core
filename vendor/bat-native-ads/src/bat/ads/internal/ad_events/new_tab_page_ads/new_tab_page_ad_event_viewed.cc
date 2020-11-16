/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_viewed.h"

#include <string>

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/frequency_capping/new_tab_page_ads/new_tab_page_ads_frequency_capping.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {
namespace new_tab_page_ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kViewed;
}  // namespace

AdEventViewed::AdEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventViewed::~AdEventViewed() = default;

void AdEventViewed::Trigger(
    const NewTabPageAdInfo& ad) {
  database::table::AdEvents database_table(ads_);
  database_table.GetAll([=](
      const Result result,
      const AdEventList& ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "New tab page ad: Failed to get ad events");
      return;
    }

    if (!ShouldConfirmAd(ad, ad_events)) {
      BLOG(1, "New tab page ad: Not allowed");
      return;
    }

    ConfirmAd(ad);
  });
}

///////////////////////////////////////////////////////////////////////////////

bool AdEventViewed::ShouldConfirmAd(
    const NewTabPageAdInfo& ad,
    const AdEventList& ad_events) {
  FrequencyCapping frequency_capping(ads_, ad_events);

  if (!frequency_capping.IsAdAllowed()) {
    return false;
  }

  if (frequency_capping.ShouldExcludeAd(ad)) {
    return false;
  }

  return true;
}

void AdEventViewed::ConfirmAd(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Viewed new tab page ad with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  AdEvents ad_events(ads_);
  ad_events.Log(ad, kConfirmationType, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log new tab page ad viewed event");
      return;
    }

    BLOG(6, "Successfully logged new tab page ad viewed event");
  });

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace new_tab_page_ads
}  // namespace ads

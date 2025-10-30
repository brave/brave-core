/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"

#include <utility>

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace ntp_background_images {

NTPSponsoredRichMediaAdEventHandler::NTPSponsoredRichMediaAdEventHandler(
    brave_ads::AdsService* ads_service)
    : ads_service_(ads_service) {}

NTPSponsoredRichMediaAdEventHandler::~NTPSponsoredRichMediaAdEventHandler() =
    default;

void NTPSponsoredRichMediaAdEventHandler::Bind(
    mojo::PendingReceiver<mojom::SponsoredRichMediaAdEventHandler>
        pending_receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(pending_receiver));
}

void NTPSponsoredRichMediaAdEventHandler::MaybeReportRichMediaAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) {
  if (!ShouldReportNewTabPageAdEvent(mojom_ad_event_type)) {
    return;
  }

  if (ads_service_) {
    // Ads service will handle the case when we should fallback to P3A and no-op
    // if the campaign should report using P3A.
    ads_service_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                           mojom_ad_metric_type,
                                           mojom_ad_event_type,
                                           /*intentional*/ base::DoNothing());
  }
}

bool NTPSponsoredRichMediaAdEventHandler::ShouldReportNewTabPageAdEvent(
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) const {
  switch (mojom_ad_event_type) {
    case brave_ads::mojom::NewTabPageAdEventType::kClicked:
    case brave_ads::mojom::NewTabPageAdEventType::kInteraction:
    case brave_ads::mojom::NewTabPageAdEventType::kMediaPlay:
    case brave_ads::mojom::NewTabPageAdEventType::kMedia25:
    case brave_ads::mojom::NewTabPageAdEventType::kMedia100: {
      return true;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kServedImpression:
    case brave_ads::mojom::NewTabPageAdEventType::kViewedImpression: {
      // Handled in `view_counter_service`.
      return false;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::NewTabPageAdEventType: "
               << base::to_underlying(mojom_ad_event_type);
}

}  // namespace ntp_background_images

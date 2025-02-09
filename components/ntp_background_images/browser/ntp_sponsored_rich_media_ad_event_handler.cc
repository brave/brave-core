/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"

#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"

namespace ntp_background_images {

NTPSponsoredRichMediaAdEventHandler::NTPSponsoredRichMediaAdEventHandler(
    brave_ads::AdsService* ads_service,
    std::unique_ptr<NTPP3AHelper> ntp_p3a_helper)
    : ads_service_(ads_service), ntp_p3a_helper_(std::move(ntp_p3a_helper)) {}

NTPSponsoredRichMediaAdEventHandler::~NTPSponsoredRichMediaAdEventHandler() =
    default;

void NTPSponsoredRichMediaAdEventHandler::Bind(
    mojo::PendingReceiver<mojom::SponsoredRichMediaAdEventHandler>
        pending_receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(pending_receiver));
}

void NTPSponsoredRichMediaAdEventHandler::ReportRichMediaAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) {
  switch (mojom_ad_event_type) {
    case brave_ads::mojom::NewTabPageAdEventType::kClicked:
    case brave_ads::mojom::NewTabPageAdEventType::kInteraction:
    case brave_ads::mojom::NewTabPageAdEventType::kMediaPlay:
    case brave_ads::mojom::NewTabPageAdEventType::kMedia25:
    case brave_ads::mojom::NewTabPageAdEventType::kMedia100: {
      MaybeRecordNewTabPageAdEvent(creative_instance_id, mojom_ad_event_type);

      MaybeTriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                    mojom_ad_event_type);

      break;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kServedImpression:
    case brave_ads::mojom::NewTabPageAdEventType::kViewedImpression: {
      // Handled in `view_counter_service`.
      break;
    }
  }
}

void NTPSponsoredRichMediaAdEventHandler::MaybeTriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) {
  if (!ads_service_) {
    return;
  }

  ads_service_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                         mojom_ad_event_type,
                                         /*intentional*/ base::DoNothing());
}

void NTPSponsoredRichMediaAdEventHandler::MaybeRecordNewTabPageAdEvent(
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) {
  if (!ntp_p3a_helper_) {
    return;
  }

  ntp_p3a_helper_->RecordNewTabPageAdEvent(mojom_ad_event_type,
                                           creative_instance_id);
}

}  // namespace ntp_background_images

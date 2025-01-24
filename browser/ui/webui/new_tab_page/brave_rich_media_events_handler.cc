/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/new_tab_page/brave_rich_media_events_handler.h"

#include <optional>

#include "base/containers/fixed_flat_map.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"

namespace {

// TODO(tmancey): @aseren should be decoupled to ads component as a util.
constexpr auto kNewTabPageAdEventTypeMap =
    base::MakeFixedFlatMap<std::string,
                           brave_ads::mojom::NewTabPageAdEventType>({
        {"click", brave_ads::mojom::NewTabPageAdEventType::kClicked},
        {"media_play", brave_ads::mojom::NewTabPageAdEventType::kMediaPlay},
        {"media_25", brave_ads::mojom::NewTabPageAdEventType::kMedia25},
        {"media_100", brave_ads::mojom::NewTabPageAdEventType::kMedia100},
    });

// TODO(tmancey): @aseren should be decoupled to
// components/brave_ads/core/public/ad_units/new_tab_page_ad as a util and
// should be changed to supported all events. I know we only support the above
// events, but see other comemnts below as the switch statements protect us
// here.
std::optional<brave_ads::mojom::NewTabPageAdEventType>
ToMojomNewTabPageAdEventType(const std::string& event_type) {
  const auto iter = kNewTabPageAdEventTypeMap.find(event_type);
  if (iter == kNewTabPageAdEventTypeMap.cend()) {
    return std::nullopt;
  }

  return iter->second;
}

}  // namespace

BraveRichMediaEventsHandler::BraveRichMediaEventsHandler(
    brave_ads::AdsService* ads_service,
    std::unique_ptr<ntp_background_images::NTPP3AHelper> ntp_p3a_helper,
    mojo::PendingReceiver<brave_new_tab_page::mojom::RichMediaEventsHandler>
        pending_receiver)
    : ads_service_(ads_service), ntp_p3a_helper_(std::move(ntp_p3a_helper)) {
  receiver_.Bind(std::move(pending_receiver));
}

BraveRichMediaEventsHandler::~BraveRichMediaEventsHandler() = default;

void BraveRichMediaEventsHandler::ReportRichMediaEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const std::string& event_type) {
  if (!ads_service_) {
    // TODO(tmancey): @aseren can you confirm P3A is sent for non-rewards
    // because we exist early if the ads service is null. And we should only
    // send confirmations if rewards are enabled. I ask because the logic below
    // seems strange.
    return;
  }

  // TODO(tmancey): No need to be optional, because if we move
  // ads_service_->TriggerNewTabPageAdEvent logic inside the switch cases it
  // would be easier to read. We likely need to check the ads_service when
  // calling it too instead of on entry to this function.
  const std::optional<brave_ads::mojom::NewTabPageAdEventType>
      mojom_ad_event_type = ToMojomNewTabPageAdEventType(event_type);
  if (!mojom_ad_event_type) {
    return;
  }

  ads_service_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                         *mojom_ad_event_type,
                                         /*intentional*/ base::DoNothing());

  if (ntp_p3a_helper_) {
    // We only P3A if Brave Rewards is disabled.
    // TODO(tmancey): Discussed with @aseren about why we do not send views,
    // this is due to them being sent elsewhere. So we shnould add a comment
    // pointing to that code. However, for consistency, why could we not move
    // that code to here, would that work? This would mean all events being
    // triggered from the same code path.
    switch (*mojom_ad_event_type) {
      case brave_ads::mojom::NewTabPageAdEventType::kClicked: {
        ntp_p3a_helper_->RecordClickAndMaybeLand(creative_instance_id);
        break;
      }

      default: {
        // Unsupported event type.
        break;
      }
    }
  }
}

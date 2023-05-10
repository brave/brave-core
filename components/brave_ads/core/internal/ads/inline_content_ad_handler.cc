/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/inline_content_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"

namespace brave_ads {

InlineContentAdHandler::InlineContentAdHandler(
    Account& account,
    Transfer& transfer,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : account_(account),
      transfer_(transfer),
      serving_(subdivision_targeting, anti_targeting_resource) {
  event_handler_.SetDelegate(this);

  serving_.SetDelegate(this);
}

InlineContentAdHandler::~InlineContentAdHandler() = default;

void InlineContentAdHandler::MaybeServe(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  serving_.MaybeServeAd(dimensions, std::move(callback));
}

void InlineContentAdHandler::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_.FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAdHandler::OnOpportunityAroseToServeInlineContentAd(
    const SegmentList& /*segments*/) {
  BLOG(1, "Opportunity arose to serve an inline content ad");
}

void InlineContentAdHandler::OnDidServeInlineContentAd(
    const InlineContentAdInfo& ad) {
  TriggerEvent(ad.placement_id, ad.creative_instance_id,
               mojom::InlineContentAdEventType::kServed);
}

void InlineContentAdHandler::OnDidFireInlineContentAdServedEvent(
    const InlineContentAdInfo& ad) {
  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void InlineContentAdHandler::OnDidFireInlineContentAdViewedEvent(
    const InlineContentAdInfo& ad) {
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kViewed);

  privacy::p2a::RecordAdImpression(ad);
}

void InlineContentAdHandler::OnDidFireInlineContentAdClickedEvent(
    const InlineContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads

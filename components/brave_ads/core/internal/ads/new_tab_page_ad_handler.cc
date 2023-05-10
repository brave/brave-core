/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdHandler::NewTabPageAdHandler(
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

NewTabPageAdHandler::~NewTabPageAdHandler() = default;

void NewTabPageAdHandler::MaybeServe(MaybeServeNewTabPageAdCallback callback) {
  serving_.MaybeServeAd(std::move(callback));
}

void NewTabPageAdHandler::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_.FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAdHandler::OnOpportunityAroseToServeNewTabPageAd(
    const SegmentList& /*segments*/) {
  BLOG(1, "Opportunity arose to serve a new tab page ad");
}

void NewTabPageAdHandler::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  TriggerEvent(ad.placement_id, ad.creative_instance_id,
               mojom::NewTabPageAdEventType::kServed);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdServedEvent(
    const NewTabPageAdInfo& ad) {
  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdViewedEvent(
    const NewTabPageAdInfo& ad) {
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kViewed);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdClickedEvent(
    const NewTabPageAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads

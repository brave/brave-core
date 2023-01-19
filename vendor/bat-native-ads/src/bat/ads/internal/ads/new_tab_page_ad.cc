/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ad.h"

#include <utility>

#include "base/check.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"
#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

NewTabPageAd::NewTabPageAd(
    Account* account,
    Transfer* transfer,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : account_(account), transfer_(transfer) {
  DCHECK(account_);
  DCHECK(transfer_);

  event_handler_ = std::make_unique<new_tab_page_ads::EventHandler>();
  event_handler_->AddObserver(this);

  serving_ = std::make_unique<new_tab_page_ads::Serving>(
      subdivision_targeting, anti_targeting_resource);
  serving_->AddObserver(this);
}

NewTabPageAd::~NewTabPageAd() {
  event_handler_->RemoveObserver(this);
  serving_->RemoveObserver(this);
}

void NewTabPageAd::MaybeServe(MaybeServeNewTabPageAdCallback callback) {
  serving_->MaybeServeAd(std::move(callback));
}

void NewTabPageAd::TriggerEvent(const std::string& placement_id,
                                const std::string& creative_instance_id,
                                const mojom::NewTabPageAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_->FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAd::OnOpportunityAroseToServeNewTabPageAd(
    const SegmentList& /*segments*/) {
  BLOG(1, "Opportunity arose to serve a new tab page ad");
}

void NewTabPageAd::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  TriggerEvent(ad.placement_id, ad.creative_instance_id,
               mojom::NewTabPageAdEventType::kServed);
}

void NewTabPageAd::OnNewTabPageAdServed(const NewTabPageAdInfo& ad) {
  ClientStateManager::GetInstance()->UpdateSeenAd(ad);
}

void NewTabPageAd::OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void NewTabPageAd::OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace ads

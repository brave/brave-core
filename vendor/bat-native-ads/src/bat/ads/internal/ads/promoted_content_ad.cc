/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/promoted_content_ad.h"

#include "base/check.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {

PromotedContentAd::PromotedContentAd(Account* account, Transfer* transfer)
    : account_(account), transfer_(transfer) {
  DCHECK(account_);
  DCHECK(transfer_);

  event_handler_ = std::make_unique<promoted_content_ads::EventHandler>();
  event_handler_->AddObserver(this);
}

PromotedContentAd::~PromotedContentAd() {
  event_handler_->RemoveObserver(this);
}

void PromotedContentAd::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_->FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAd::OnPromotedContentAdViewed(
    const PromotedContentAdInfo& ad) {
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void PromotedContentAd::OnPromotedContentAdClicked(
    const PromotedContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace ads

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/promoted_content_ad_handler.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/promoted_content_ad_info.h"

namespace brave_ads {

PromotedContentAd::PromotedContentAd(Account& account, Transfer& transfer)
    : account_(account), transfer_(transfer) {
  event_handler_.SetDelegate(this);
}

PromotedContentAd::~PromotedContentAd() = default;

void PromotedContentAd::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_.FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAd::OnDidFirePromotedContentAdViewedEvent(
    const PromotedContentAdInfo& ad) {
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kViewed);
}

void PromotedContentAd::OnDidFirePromotedContentAdClickedEvent(
    const PromotedContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads

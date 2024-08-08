/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

Reactions::Reactions() {
  AdHistoryManager::GetInstance().AddObserver(this);
}

Reactions::~Reactions() {
  AdHistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reactions::OnDidLikeAd(const AdHistoryItemInfo& ad_history_item) {
  GetAccount().Deposit(ad_history_item.creative_instance_id,
                       ad_history_item.segment, ad_history_item.type,
                       ConfirmationType::kLikedAd);
}

void Reactions::OnDidDislikeAd(const AdHistoryItemInfo& ad_history_item) {
  GetAccount().Deposit(ad_history_item.creative_instance_id,
                       ad_history_item.segment, ad_history_item.type,
                       ConfirmationType::kDislikedAd);
}

void Reactions::OnDidMarkAdAsInappropriate(
    const AdHistoryItemInfo& ad_history_item) {
  GetAccount().Deposit(ad_history_item.creative_instance_id,
                       ad_history_item.segment, ad_history_item.type,
                       ConfirmationType::kMarkAdAsInappropriate);
}

void Reactions::OnDidSaveAd(const AdHistoryItemInfo& ad_history_item) {
  GetAccount().Deposit(ad_history_item.creative_instance_id,
                       ad_history_item.segment, ad_history_item.type,
                       ConfirmationType::kSavedAd);
}

}  // namespace brave_ads

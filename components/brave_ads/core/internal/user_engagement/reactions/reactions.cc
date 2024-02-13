/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_content_info.h"

namespace brave_ads {

Reactions::Reactions(Account& account) : account_(account) {
  HistoryManager::GetInstance().AddObserver(this);
}

Reactions::~Reactions() {
  HistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reactions::OnDidLikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.segment,
                    ad_content.type, ConfirmationType::kLikedAd);
}

void Reactions::OnDidDislikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.segment,
                    ad_content.type, ConfirmationType::kDislikedAd);
}

void Reactions::OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.segment,
                    ad_content.type, ConfirmationType::kMarkAdAsInappropriate);
}

void Reactions::OnDidSaveAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.segment,
                    ad_content.type, ConfirmationType::kSavedAd);
}

}  // namespace brave_ads

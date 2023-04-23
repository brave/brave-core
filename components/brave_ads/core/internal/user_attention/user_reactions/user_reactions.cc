/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_reactions/user_reactions.h"

#include "brave/components/brave_ads/core/ad_content_info.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"

namespace brave_ads {

UserReactions::UserReactions(Account& account) : account_(account) {
  HistoryManager::GetInstance().AddObserver(this);
}

UserReactions::~UserReactions() {
  HistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void UserReactions::OnDidLikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ad_content.segment, ConfirmationType::kUpvoted);
}

void UserReactions::OnDidDislikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ad_content.segment, ConfirmationType::kDownvoted);
}

void UserReactions::OnDidMarkAdAsInappropriate(
    const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ad_content.segment, ConfirmationType::kFlagged);
}

void UserReactions::OnDidSaveAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ad_content.segment, ConfirmationType::kSaved);
}

}  // namespace brave_ads

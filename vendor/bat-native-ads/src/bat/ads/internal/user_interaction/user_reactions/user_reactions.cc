/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_reactions/user_reactions.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/history/history_manager.h"

namespace ads {

UserReactions::UserReactions(Account* account) : account_(account) {
  DCHECK(account_);

  HistoryManager::GetInstance()->AddObserver(this);
}

UserReactions::~UserReactions() {
  HistoryManager::GetInstance()->RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void UserReactions::OnDidLikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kUpvoted);
}

void UserReactions::OnDidDislikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kDownvoted);
}

void UserReactions::OnDidMarkAdAsInappropriate(
    const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kFlagged);
}

void UserReactions::OnDidSaveAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kSaved);
}

}  // namespace ads

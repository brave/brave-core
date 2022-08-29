/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/reactions/reactions.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/history/history_manager.h"

namespace ads {

Reactions::Reactions(Account* account) : account_(account) {
  DCHECK(account_);

  HistoryManager::GetInstance()->AddObserver(this);
}

Reactions::~Reactions() {
  HistoryManager::GetInstance()->RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reactions::OnDidLikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kUpvoted);
}

void Reactions::OnDidDislikeAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kDownvoted);
}

void Reactions::OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kFlagged);
}

void Reactions::OnDidSaveAd(const AdContentInfo& ad_content) {
  account_->Deposit(ad_content.creative_instance_id, ad_content.type,
                    ConfirmationType::kSaved);
}

}  // namespace ads

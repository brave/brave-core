/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ad.h"

#include "base/check.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/transfer/transfer.h"

namespace ads {

SearchResultAd::SearchResultAd(Account* account, Transfer* transfer)
    : account_(account), transfer_(transfer) {
  DCHECK(account_);
  DCHECK(transfer_);

  event_handler_ = std::make_unique<search_result_ads::EventHandler>();
  event_handler_->AddObserver(this);
}

SearchResultAd::~SearchResultAd() {
  event_handler_->RemoveObserver(this);
}

void SearchResultAd::TriggerEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_->FireEvent(
      ad_mojom, event_type,
      [callback](const bool success, const std::string& placement_id,
                 const mojom::SearchResultAdEventType event_type) {
        DCHECK(ads::mojom::IsKnownEnumValue(event_type));

        callback(success, placement_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void SearchResultAd::OnSearchResultAdViewed(const SearchResultAdInfo& ad) {
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);
}

void SearchResultAd::OnSearchResultAdClicked(const SearchResultAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace ads

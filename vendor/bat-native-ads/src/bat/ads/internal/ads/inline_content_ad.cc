/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/inline_content_ad.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler.h"
#include "bat/ads/internal/ads/serving/inline_content_ad_serving.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"
#include "bat/ads/internal/transfer/transfer.h"

namespace ads {

InlineContentAd::InlineContentAd(
    Account* account,
    Transfer* transfer,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : account_(account), transfer_(transfer) {
  DCHECK(account_);
  DCHECK(transfer_);

  event_handler_ = std::make_unique<inline_content_ads::EventHandler>();

  serving_ = std::make_unique<inline_content_ads::Serving>(
      subdivision_targeting, anti_targeting_resource);

  event_handler_->AddObserver(this);
  serving_->AddObserver(this);
}

InlineContentAd::~InlineContentAd() {
  event_handler_->RemoveObserver(this);
  serving_->RemoveObserver(this);
}

void InlineContentAd::MaybeServe(const std::string& dimensions,
                                 MaybeServeInlineContentAdCallback callback) {
  serving_->MaybeServeAd(dimensions,
                         [=](const bool success, const std::string& dimensions,
                             const InlineContentAdInfo& ad) {
                           callback(success, dimensions, ad);
                         });
}

void InlineContentAd::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  event_handler_->FireEvent(placement_id, creative_instance_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAd::OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {
  event_handler_->FireEvent(ad.placement_id, ad.creative_instance_id,
                            mojom::InlineContentAdEventType::kServed);
}

void InlineContentAd::OnInlineContentAdViewed(const InlineContentAdInfo& ad) {
  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);

  privacy::p2a::RecordAdImpression(ad);
}

void InlineContentAd::OnInlineContentAdClicked(const InlineContentAdInfo& ad) {
  transfer_->SetLastClickedAd(ad);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace ads

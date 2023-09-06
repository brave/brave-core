/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/ad_handler.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_info.h"

namespace brave_ads {

AdHandler::AdHandler(Account& account)
    : account_(account),
      purchase_intent_processor_(purchase_intent_resource_),
      epsilon_greedy_bandit_resource_(catalog_),
      text_classification_processor_(text_classification_resource_),
      text_embedding_processor_(text_embedding_resource_),
      inline_content_ad_handler_(account,
                                 transfer_,
                                 subdivision_targeting_,
                                 anti_targeting_resource_),
      new_tab_page_ad_handler_(account,
                               transfer_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      notification_ad_handler_(account,
                               transfer_,
                               epsilon_greedy_bandit_processor_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      promoted_content_ad_handler_(account, transfer_),
      search_result_ad_handler_(account, transfer_) {
  conversions_.AddObserver(this);
  transfer_.AddObserver(this);
}

AdHandler::~AdHandler() {
  conversions_.RemoveObserver(this);
  transfer_.RemoveObserver(this);
}

void AdHandler::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());
  CHECK(mojom::IsKnownEnumValue(event_type));

  notification_ad_handler_.TriggerEvent(placement_id, event_type,
                                        std::move(callback));
}

void AdHandler::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  new_tab_page_ad_handler_.MaybeServe(std::move(callback));
}

void AdHandler::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());
  CHECK(!creative_instance_id.empty());
  CHECK(mojom::IsKnownEnumValue(event_type));

  new_tab_page_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                        event_type, std::move(callback));
}

void AdHandler::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());
  CHECK(!creative_instance_id.empty());
  CHECK(mojom::IsKnownEnumValue(event_type));

  promoted_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                            event_type, std::move(callback));
}

void AdHandler::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  CHECK(!dimensions.empty());

  inline_content_ad_handler_.MaybeServe(dimensions, std::move(callback));
}

void AdHandler::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());
  CHECK(!creative_instance_id.empty());
  CHECK(mojom::IsKnownEnumValue(event_type));

  inline_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                          event_type, std::move(callback));
}

void AdHandler::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(ad_mojom);
  CHECK(mojom::IsKnownEnumValue(event_type));

  search_result_ad_handler_.TriggerEvent(std::move(ad_mojom), event_type,
                                         std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void AdHandler::OnDidConvertAd(const ConversionInfo& conversion) {
  CHECK(conversion.IsValid());

  account_->Deposit(conversion.creative_instance_id, conversion.segment,
                    conversion.ad_type, ConfirmationType::kConversion);
}

void AdHandler::OnDidTransferAd(const AdInfo& ad) {
  CHECK(ad.IsValid());

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kTransferred);
}

}  // namespace brave_ads

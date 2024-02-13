/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

AdHandler::AdHandler(Account& account)
    : account_(account),
      purchase_intent_processor_(purchase_intent_resource_),
      epsilon_greedy_bandit_resource_(catalog_),
      text_classification_processor_(text_classification_resource_),
      text_embedding_processor_(text_embedding_resource_),
      inline_content_ad_handler_(account,
                                 site_visit_,
                                 subdivision_targeting_,
                                 anti_targeting_resource_),
      new_tab_page_ad_handler_(account,
                               site_visit_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      notification_ad_handler_(account,
                               site_visit_,
                               epsilon_greedy_bandit_processor_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      promoted_content_ad_handler_(account, site_visit_),
      search_result_ad_handler_(account, site_visit_) {
  conversions_.AddObserver(this);
  site_visit_.AddObserver(this);
  subdivision_.AddObserver(&country_code_);
  subdivision_.AddObserver(&subdivision_targeting_);
}

AdHandler::~AdHandler() {
  conversions_.RemoveObserver(this);
  site_visit_.RemoveObserver(this);
  subdivision_.RemoveObserver(&country_code_);
  subdivision_.RemoveObserver(&subdivision_targeting_);
}

void AdHandler::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

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

  new_tab_page_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                        event_type, std::move(callback));
}

void AdHandler::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

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

  inline_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                          event_type, std::move(callback));
}

void AdHandler::TriggerSearchResultAdEvent(
    mojom::SearchResultAdInfoPtr ad_mojom,
    const mojom::SearchResultAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(ad_mojom);

  search_result_ad_handler_.TriggerEvent(std::move(ad_mojom), event_type,
                                         std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void AdHandler::OnDidConvertAd(const ConversionInfo& conversion) {
  CHECK(conversion.IsValid());

  account_->Deposit(conversion.creative_instance_id, conversion.segment,
                    conversion.ad_type, ConfirmationType::kConversion);
}

void AdHandler::OnMaybeLandOnPage(const AdInfo& ad, const base::Time maybe_at) {
  CHECK(ad.IsValid());

  BLOG(1, "Maybe land page for " << ad.target_url << " "
                                 << FriendlyDateAndTime(maybe_at));
}

void AdHandler::OnDidLandOnPage(const AdInfo& ad) {
  CHECK(ad.IsValid());

  BLOG(1, "Landed on page for " << ad.target_url);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kLanded);
}

void AdHandler::OnDidNotLandOnPage(const AdInfo& ad) {
  CHECK(ad.IsValid());

  BLOG(1, "Did not land on page for " << ad.target_url);
}

void AdHandler::OnCanceledPageLand(const AdInfo& ad, int32_t tab_id) {
  CHECK(ad.IsValid());

  BLOG(1, "Canceled page land for creative instance id "
              << ad.creative_instance_id << " with tab id " << tab_id);
}

}  // namespace brave_ads

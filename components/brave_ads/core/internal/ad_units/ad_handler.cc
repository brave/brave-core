/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/page_land_user_data.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

AdHandler::AdHandler()
    : purchase_intent_processor_(purchase_intent_resource_),
      text_classification_processor_(text_classification_resource_),
      inline_content_ad_handler_(site_visit_,
                                 subdivision_targeting_,
                                 anti_targeting_resource_),
      new_tab_page_ad_handler_(site_visit_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      notification_ad_handler_(site_visit_,
                               subdivision_targeting_,
                               anti_targeting_resource_),
      promoted_content_ad_handler_(site_visit_),
      search_result_ad_handler_(site_visit_) {
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
    const mojom::NotificationAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

  notification_ad_handler_.TriggerEvent(placement_id, mojom_ad_event_type,
                                        std::move(callback));
}

void AdHandler::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  new_tab_page_ad_handler_.MaybeServe(std::move(callback));
}

void AdHandler::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

  new_tab_page_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                        mojom_ad_event_type,
                                        std::move(callback));
}

void AdHandler::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

  promoted_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                            mojom_ad_event_type,
                                            std::move(callback));
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
    const mojom::InlineContentAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(!placement_id.empty());

  inline_content_ad_handler_.TriggerEvent(placement_id, creative_instance_id,
                                          mojom_ad_event_type,
                                          std::move(callback));
}

void AdHandler::TriggerSearchResultAdEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const mojom::SearchResultAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom_creative_ad);

  search_result_ad_handler_.TriggerEvent(
      std::move(mojom_creative_ad), mojom_ad_event_type, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void AdHandler::OnDidConvertAd(const ConversionInfo& conversion) {
  CHECK(conversion.IsValid());

  BLOG(1, "Converted " << ToString(conversion.action_type) << " "
                       << ConversionTypeToString(conversion) << " for "
                       << conversion.ad_type << " with creative instance id "
                       << conversion.creative_instance_id
                       << ", creative set id " << conversion.creative_set_id
                       << ", campaign id " << conversion.campaign_id
                       << " and advertiser id " << conversion.advertiser_id);

  GetAccount().DepositWithUserData(
      conversion.creative_instance_id, conversion.segment, conversion.ad_type,
      ConfirmationType::kConversion, BuildConversionUserData(conversion));
}

void AdHandler::OnMaybeLandOnPage(const AdInfo& ad,
                                  const base::TimeDelta after) {
  CHECK(ad.IsValid());

  BLOG(1, "Maybe land on page for " << ad.target_url << " in " << after);
}

void AdHandler::OnDidSuspendPageLand(const TabInfo& tab,
                                     const base::TimeDelta remaining_time) {
  BLOG(1, "Suspended page landing on tab id "
              << tab.id << " with " << remaining_time << " remaining");
}

void AdHandler::OnDidResumePageLand(const TabInfo& tab,
                                    const base::TimeDelta remaining_time) {
  BLOG(1, "Resumed page landing on tab id " << tab.id << " and maybe land in "
                                            << remaining_time);
}

void AdHandler::OnDidLandOnPage(const TabInfo& tab, const AdInfo& ad) {
  CHECK(ad.IsValid());

  BLOG(1, "Landed on page for " << ad.target_url << " on tab id " << tab.id);

  GetAccount().DepositWithUserData(ad.creative_instance_id, ad.segment, ad.type,
                                   ConfirmationType::kLanded,
                                   BuildPageLandUserData(tab));
}

void AdHandler::OnDidNotLandOnPage(const TabInfo& tab, const AdInfo& ad) {
  CHECK(ad.IsValid());

  BLOG(1,
       "Did not land on page for " << ad.target_url << " on tab id " << tab.id);
}

void AdHandler::OnCanceledPageLand(const int32_t tab_id, const AdInfo& ad) {
  CHECK(ad.IsValid());

  BLOG(1, "Canceled page land for creative instance id "
              << ad.creative_instance_id << " on tab id " << tab_id);
}

}  // namespace brave_ads

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_HANDLER_H_

#include <cstdint>
#include <string>

#include "brave/components/brave_ads/core/internal/ad_units/creative_ad_cache.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ad_units/notification_ad/notification_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_handler.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/common/country_code/country_code.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class SiteVisit;
struct AdInfo;
struct ConversionInfo;

class AdHandler final : public ConversionsObserver, SiteVisitObserver {
 public:
  AdHandler();

  AdHandler(const AdHandler&) = delete;
  AdHandler& operator=(const AdHandler&) = delete;

  ~AdHandler() override;

  void MaybeServeInlineContentAd(const std::string& dimensions,
                                 MaybeServeInlineContentAdCallback callback);
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback);
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

  std::optional<mojom::CreativeSearchResultAdInfoPtr> MaybeGetSearchResultAd(
      const std::string& placement_id);

  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback);

 private:
  // ConversionsObserver:
  void OnDidConvertAd(const ConversionInfo& conversion) override;

  // SiteVisitObserver:
  void OnMaybeLandOnPage(const AdInfo& ad, base::TimeDelta after) override;
  void OnDidSuspendPageLand(int32_t tab_id,
                            base::TimeDelta remaining_time) override;
  void OnDidResumePageLand(int32_t tab_id,
                           base::TimeDelta remaining_time) override;
  void OnDidLandOnPage(int32_t tab_id,
                       int32_t http_response_code,
                       const AdInfo& ad) override;
  void OnDidNotLandOnPage(int32_t tab_id, const AdInfo& ad) override;
  void OnCanceledPageLand(int32_t tab_id, const AdInfo& ad) override;

  Catalog catalog_;

  CreativeAdCache creative_ad_cache_;

  Conversions conversions_;

  SiteVisit site_visit_;

  CountryCode country_code_;
  SubdivisionTargeting subdivision_targeting_;
  Subdivision subdivision_;

  AntiTargetingResource anti_targeting_resource_;

  PurchaseIntentResource purchase_intent_resource_;
  PurchaseIntentProcessor purchase_intent_processor_;

  TextClassificationResource text_classification_resource_;
  TextClassificationProcessor text_classification_processor_;

  InlineContentAdHandler inline_content_ad_handler_;
  NewTabPageAdHandler new_tab_page_ad_handler_;
  NotificationAdHandler notification_ad_handler_;
  PromotedContentAdHandler promoted_content_ad_handler_;
  SearchResultAdHandler search_result_ad_handler_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_AD_HANDLER_H_

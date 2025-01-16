/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_factory.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/inline_content_ads/inline_content_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

InlineContentAdServing::InlineContentAdServing(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource) {
  eligible_ads_ = EligibleInlineContentAdsFactory::Build(
      kInlineContentAdServingVersion.Get(), subdivision_targeting,
      anti_targeting_resource);
}

InlineContentAdServing::~InlineContentAdServing() {
  delegate_ = nullptr;
}

void InlineContentAdServing::MaybeServeAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetVisible();
  if (!tab) {
    BLOG(1, "Inline content ad not served: No visible tab found");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  GetAdEvents(tab->id, dimensions, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

bool InlineContentAdServing::CanServeAd(const AdEventList& ad_events) const {
  if (!base::FeatureList::IsEnabled(kInlineContentAdServingFeature)) {
    BLOG(1, "Inline content ad not served: Feature is disabled");
    return false;
  }

  if (!IsSupported()) {
    BLOG(1, "Inline content ad not served: Unsupported version");
    return false;
  }

  if (!InlineContentAdPermissionRules::HasPermission(ad_events)) {
    BLOG(1,
         "Inline content ad not served: Not allowed due to permission rules");
    return false;
  }

  return true;
}

void InlineContentAdServing::GetAdEvents(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  const database::table::AdEvents database_table;
  database_table.Get(
      mojom::AdType::kInlineContentAd,
      mojom::ConfirmationType::kServedImpression, /*time_window=*/base::Days(1),
      base::BindOnce(&InlineContentAdServing::GetAdEventsCallback,
                     weak_factory_.GetWeakPtr(), tab_id, dimensions,
                     std::move(callback)));
}

void InlineContentAdServing::GetAdEventsCallback(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Inline content ad not served: Failed to get ad events");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  if (!CanServeAd(ad_events)) {
    BLOG(1, "Inline content ad not served: Not allowed");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  GetUserModel(tab_id, dimensions, std::move(callback));
}

void InlineContentAdServing::GetUserModel(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory, "InlineContentAdServing::GetUserModel",
      TRACE_ID_WITH_SCOPE("InlineContentAdServing", trace_id));

  BuildUserModel(base::BindOnce(&InlineContentAdServing::GetUserModelCallback,
                                weak_factory_.GetWeakPtr(), tab_id, dimensions,
                                std::move(callback), trace_id));
}

void InlineContentAdServing::GetUserModelCallback(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    uint64_t trace_id,
    UserModelInfo user_model) const {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      kTraceEventCategory, "InlineContentAdServing::GetUserModel",
      TRACE_ID_WITH_SCOPE("InlineContentAdServing", trace_id));

  NotifyOpportunityAroseToServeInlineContentAd();

  GetEligibleAds(tab_id, dimensions, std::move(callback),
                 std::move(user_model));
}

void InlineContentAdServing::GetEligibleAds(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    UserModelInfo user_model) const {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory, "InlineContentAdServing::GetEligibleAds",
      TRACE_ID_WITH_SCOPE("InlineContentAdServing", trace_id));

  eligible_ads_->GetForUserModel(
      std::move(user_model), dimensions,
      base::BindOnce(&InlineContentAdServing::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), tab_id, dimensions,
                     std::move(callback), trace_id));
}

void InlineContentAdServing::GetEligibleAdsCallback(
    int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    uint64_t trace_id,
    const CreativeInlineContentAdList& creative_ads) const {
  TRACE_EVENT_NESTABLE_ASYNC_END1(
      kTraceEventCategory, "InlineContentAdServing::GetEligibleAds",
      TRACE_ID_WITH_SCOPE("InlineContentAdServing", trace_id), "creative_ads",
      creative_ads.size());

  if (creative_ads.empty()) {
    BLOG(1, "Inline content ad not served: No eligible ads found");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const CreativeInlineContentAdInfo creative_ad =
      ChooseCreativeAd(creative_ads);
  BLOG(1, "Chosen eligible ad with creative instance id "
              << creative_ad.creative_instance_id << " and a priority of "
              << creative_ad.priority);

  ServeAd(tab_id, BuildInlineContentAd(creative_ad), std::move(callback));
}

void InlineContentAdServing::ServeAd(
    int32_t tab_id,
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback) const {
  if (!ad.IsValid()) {
    BLOG(1, "Inline content ad not served: Invalid ad");
    return FailedToServeAd(ad.dimensions, std::move(callback));
  }

  eligible_ads_->SetLastServedAd(ad);

  SuccessfullyServedAd(tab_id, ad, std::move(callback));
}

void InlineContentAdServing::SuccessfullyServedAd(
    int32_t tab_id,
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback) const {
  NotifyDidServeInlineContentAd(tab_id, ad);

  std::move(callback).Run(ad.dimensions, ad);
}

void InlineContentAdServing::FailedToServeAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) const {
  NotifyFailedToServeInlineContentAd();

  std::move(callback).Run(dimensions, /*ad=*/std::nullopt);
}

void InlineContentAdServing::NotifyOpportunityAroseToServeInlineContentAd()
    const {
  if (delegate_) {
    delegate_->OnOpportunityAroseToServeInlineContentAd();
  }
}

void InlineContentAdServing::NotifyDidServeInlineContentAd(
    int32_t tab_id,
    const InlineContentAdInfo& ad) const {
  if (delegate_) {
    delegate_->OnDidServeInlineContentAd(tab_id, ad);
  }
}

void InlineContentAdServing::NotifyFailedToServeInlineContentAd() const {
  if (delegate_) {
    delegate_->OnFailedToServeInlineContentAd();
  }
}

}  // namespace brave_ads

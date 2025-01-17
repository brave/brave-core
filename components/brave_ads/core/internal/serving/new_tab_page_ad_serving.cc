/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_id_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_factory.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

NewTabPageAdServing::NewTabPageAdServing(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource) {
  eligible_ads_ =
      EligibleAdsFactory::Build(kNewTabPageAdServingVersion.Get(),
                                subdivision_targeting, anti_targeting_resource);
}

NewTabPageAdServing::~NewTabPageAdServing() {
  delegate_ = nullptr;
}

void NewTabPageAdServing::MaybeServeAd(
    MaybeServeNewTabPageAdCallback callback) {
  GetAdEvents(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

bool NewTabPageAdServing::CanServeAd(const AdEventList& ad_events) const {
  if (!base::FeatureList::IsEnabled(kNewTabPageAdServingFeature)) {
    BLOG(1, "New tab page ad not served: Feature is disabled");
    return false;
  }

  if (!IsSupported()) {
    BLOG(1, "New tab page ad not served: Unsupported version");
    return false;
  }

  if (!NewTabPageAdPermissionRules::HasPermission(ad_events)) {
    BLOG(1, "New tab page ad not served: Not allowed due to permission rules");
    return false;
  }

  return true;
}

void NewTabPageAdServing::GetAdEvents(MaybeServeNewTabPageAdCallback callback) {
  const database::table::AdEvents database_table;
  database_table.Get(
      mojom::AdType::kNewTabPageAd, mojom::ConfirmationType::kServedImpression,
      /*time_window=*/base::Days(1),
      base::BindOnce(&NewTabPageAdServing::GetAdEventsCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NewTabPageAdServing::GetAdEventsCallback(
    MaybeServeNewTabPageAdCallback callback,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "New tab page ad not served: Failed to get ad events");
    return FailedToServeAd(std::move(callback));
  }

  if (!CanServeAd(ad_events)) {
    BLOG(1, "New tab page ad not served: Not allowed");
    return FailedToServeAd(std::move(callback));
  }

  GetUserModel(std::move(callback));
}

void NewTabPageAdServing::GetUserModel(
    MaybeServeNewTabPageAdCallback callback) {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory, "NewTabPageAdServing::GetUserModel",
      TRACE_ID_WITH_SCOPE("NewTabPageAdServing", trace_id));

  BuildUserModel(base::BindOnce(&NewTabPageAdServing::GetUserModelCallback,
                                weak_factory_.GetWeakPtr(), std::move(callback),
                                trace_id));
}

void NewTabPageAdServing::GetUserModelCallback(
    MaybeServeNewTabPageAdCallback callback,
    uint64_t trace_id,
    UserModelInfo user_model) const {
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      kTraceEventCategory, "NewTabPageAdServing::GetUserModel",
      TRACE_ID_WITH_SCOPE("NewTabPageAdServing", trace_id));

  NotifyOpportunityAroseToServeNewTabPageAd();

  GetEligibleAds(std::move(callback), std::move(user_model));
}

void NewTabPageAdServing::GetEligibleAds(
    MaybeServeNewTabPageAdCallback callback,
    UserModelInfo user_model) const {
  const uint64_t trace_id = base::trace_event::GetNextGlobalTraceId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      kTraceEventCategory, "NewTabPageAdServing::GetEligibleAds",
      TRACE_ID_WITH_SCOPE("NewTabPageAdServing", trace_id));

  eligible_ads_->GetForUserModel(
      std::move(user_model),
      base::BindOnce(&NewTabPageAdServing::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     trace_id));
}

void NewTabPageAdServing::GetEligibleAdsCallback(
    MaybeServeNewTabPageAdCallback callback,
    uint64_t trace_id,
    const CreativeNewTabPageAdList& creative_ads) const {
  TRACE_EVENT_NESTABLE_ASYNC_END1(
      kTraceEventCategory, "NewTabPageAdServing::GetEligibleAds",
      TRACE_ID_WITH_SCOPE("NewTabPageAdServing", trace_id), "creative_ads",
      creative_ads.size());

  if (creative_ads.empty()) {
    BLOG(1, "New tab page ad not served: No eligible ads found");
    return FailedToServeAd(std::move(callback));
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const CreativeNewTabPageAdInfo creative_ad = ChooseCreativeAd(creative_ads);
  BLOG(1, "Chosen eligible ad with creative instance id "
              << creative_ad.creative_instance_id << " and a priority of "
              << creative_ad.priority);

  ServeAd(BuildNewTabPageAd(creative_ad), std::move(callback));
}

void NewTabPageAdServing::ServeAd(
    const NewTabPageAdInfo& ad,
    MaybeServeNewTabPageAdCallback callback) const {
  if (!ad.IsValid()) {
    BLOG(1, "New tab page ad not served: Invalid ad");
    return FailedToServeAd(std::move(callback));
  }

  eligible_ads_->SetLastServedAd(ad);

  SuccessfullyServedAd(ad, std::move(callback));
}

void NewTabPageAdServing::SuccessfullyServedAd(
    const NewTabPageAdInfo& ad,
    MaybeServeNewTabPageAdCallback callback) const {
  NotifyDidServeNewTabPageAd(ad);

  std::move(callback).Run(ad);
}

void NewTabPageAdServing::FailedToServeAd(
    MaybeServeNewTabPageAdCallback callback) const {
  NotifyFailedToServeNewTabPageAd();

  std::move(callback).Run(/*ad=*/std::nullopt);
}

void NewTabPageAdServing::NotifyOpportunityAroseToServeNewTabPageAd() const {
  if (delegate_) {
    delegate_->OnOpportunityAroseToServeNewTabPageAd();
  }
}

void NewTabPageAdServing::NotifyDidServeNewTabPageAd(
    const NewTabPageAdInfo& ad) const {
  if (delegate_) {
    delegate_->OnDidServeNewTabPageAd(ad);
  }
}

void NewTabPageAdServing::NotifyFailedToServeNewTabPageAd() const {
  if (delegate_) {
    delegate_->OnFailedToServeNewTabPageAd();
  }
}

}  // namespace brave_ads

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
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
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

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
  const auto result = CanServeAd();
  if (!result.has_value()) {
    BLOG(1, result.error());
    return FailedToServeAd(std::move(callback));
  }

  NotifyOpportunityAroseToServeNewTabPageAd();

  GetUserModel(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

base::expected<void, std::string> NewTabPageAdServing::CanServeAd() const {
  if (!base::FeatureList::IsEnabled(kNewTabPageAdServingFeature)) {
    return base::unexpected("New tab page ad not served: Feature is disabled");
  }

  if (!IsSupported()) {
    return base::unexpected("New tab page ad not served: Unsupported version");
  }

  if (!NewTabPageAdPermissionRules::HasPermission()) {
    return base::unexpected(
        "New tab page ad not served: Not allowed due to permission rules");
  }

  return base::ok();
}

void NewTabPageAdServing::GetUserModel(
    MaybeServeNewTabPageAdCallback callback) {
  BuildUserModel(base::BindOnce(&NewTabPageAdServing::GetEligibleAds,
                                weak_factory_.GetWeakPtr(),
                                std::move(callback)));
}

void NewTabPageAdServing::GetEligibleAds(
    MaybeServeNewTabPageAdCallback callback,
    UserModelInfo user_model) const {
  eligible_ads_->GetForUserModel(
      std::move(user_model),
      base::BindOnce(&NewTabPageAdServing::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NewTabPageAdServing::GetEligibleAdsCallback(
    MaybeServeNewTabPageAdCallback callback,
    const CreativeNewTabPageAdList& creative_ads) const {
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
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid new tab page ad");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to serve new tab page ad due to the ad being invalid");

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

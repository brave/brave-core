/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_factory.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

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
  if (!IsNewTabPageAdServingFeatureEnabled()) {
    BLOG(1, "New tab page ad not served: Feature is disabled");
    return FailedToServeAd(std::move(callback));
  }

  if (!IsSupported()) {
    BLOG(1, "New tab page ad not served: Unsupported version");
    return FailedToServeAd(std::move(callback));
  }

  if (!NewTabPageAdPermissionRules::HasPermission()) {
    BLOG(1, "New tab page ad not served: Not allowed due to permission rules");
    return FailedToServeAd(std::move(callback));
  }

  BuildUserModel(base::BindOnce(&NewTabPageAdServing::OnBuildUserModel,
                                weak_factory_.GetWeakPtr(),
                                std::move(callback)));
}

void NewTabPageAdServing::OnBuildUserModel(
    MaybeServeNewTabPageAdCallback callback,
    const UserModelInfo& user_model) {
  CHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, base::BindOnce(&NewTabPageAdServing::OnGetForUserModel,
                                 weak_factory_.GetWeakPtr(),
                                 std::move(callback), user_model));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAdServing::OnGetForUserModel(
    MaybeServeNewTabPageAdCallback callback,
    const UserModelInfo& user_model,
    const bool had_opportunity,
    const CreativeNewTabPageAdList& creative_ads) {
  if (had_opportunity) {
    if (delegate_) {
      delegate_->OnOpportunityAroseToServeNewTabPageAd(
          GetTopChildSegments(user_model));
    }
  }

  if (creative_ads.empty()) {
    BLOG(1, "New tab page ad not served: No eligible ads found");
    return FailedToServeAd(std::move(callback));
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, static_cast<int>(creative_ads.size()) - 1);
  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.at(rand);

  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);
  ServeAd(ad, std::move(callback));
}

void NewTabPageAdServing::ServeAd(const NewTabPageAdInfo& ad,
                                  MaybeServeNewTabPageAdCallback callback) {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve new tab page ad");
    return FailedToServeAd(std::move(callback));
  }

  BLOG(1, "Served new tab page ad:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  companyName: " << ad.company_name << "\n"
              << "  imageUrl: " << ad.image_url << "\n"
              << "  alt: " << ad.alt << "\n"
              << "  targetUrl: " << ad.target_url << "\n"
              << "  wallpaper:\n"
              << "    imageUrl: " << ad.wallpapers[0].image_url << "\n"
              << "    focalPoint:\n"
              << "      x: " << ad.wallpapers[0].focal_point.x << "\n"
              << "      y: " << ad.wallpapers[0].focal_point.y);

  CHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  if (delegate_) {
    delegate_->OnDidServeNewTabPageAd(ad);
  }

  std::move(callback).Run(ad);
}

void NewTabPageAdServing::FailedToServeAd(
    MaybeServeNewTabPageAdCallback callback) {
  if (delegate_) {
    delegate_->OnFailedToServeNewTabPageAd();
  }

  std::move(callback).Run(/*ad*/ absl::nullopt);
}

}  // namespace brave_ads

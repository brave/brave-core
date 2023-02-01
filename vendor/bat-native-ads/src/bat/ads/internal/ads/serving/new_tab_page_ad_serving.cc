/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving.h"

#include <utility>

#include "base/check.h"
#include "base/rand_util.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads/serving/targeting/top_segments.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_builder.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ads_features.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads::new_tab_page_ads {

Serving::Serving(geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = ::ads::features::GetServingVersion();
  eligible_ads_ = EligibleAdsFactory::Build(version, subdivision_targeting,
                                            anti_targeting_resource);
}

Serving::~Serving() = default;

void Serving::AddObserver(ServingObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Serving::RemoveObserver(ServingObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Serving::MaybeServeAd(MaybeServeNewTabPageAdCallback callback) {
  if (!features::IsEnabled()) {
    BLOG(1, "New tab page ad not served: Feature is disabled");
    FailedToServeAd(std::move(callback));
    return;
  }

  if (!IsSupported()) {
    BLOG(1, "New tab page ad not served: Unsupported version");
    FailedToServeAd(std::move(callback));
    return;
  }

  if (!PermissionRules::HasPermission()) {
    BLOG(1, "New tab page ad not served: Not allowed due to permission rules");
    FailedToServeAd(std::move(callback));
    return;
  }

  const targeting::UserModelInfo user_model = targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model,
      base::BindOnce(&Serving::OnGetForUserModel, base::Unretained(this),
                     std::move(callback), user_model));
}

void Serving::OnGetForUserModel(MaybeServeNewTabPageAdCallback callback,
                                const targeting::UserModelInfo& user_model,
                                const bool had_opportunity,
                                const CreativeNewTabPageAdList& creative_ads) {
  if (had_opportunity) {
    const SegmentList segments = targeting::GetTopChildSegments(user_model);
    NotifyOpportunityAroseToServeNewTabPageAd(segments);
  }

  if (creative_ads.empty()) {
    BLOG(1, "New tab page ad not served: No eligible ads found");
    FailedToServeAd(std::move(callback));
    return;
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, static_cast<int>(creative_ads.size()) - 1);
  const CreativeNewTabPageAdInfo& creative_ad = creative_ads.at(rand);

  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);
  ServeAd(ad, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

bool Serving::IsSupported() const {
  return static_cast<bool>(eligible_ads_);
}

void Serving::ServeAd(const NewTabPageAdInfo& ad,
                      MaybeServeNewTabPageAdCallback callback) {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve new tab page ad");
    FailedToServeAd(std::move(callback));
    return;
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

  DCHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  NotifyDidServeNewTabPageAd(ad);

  std::move(callback).Run(ad);
}

void Serving::FailedToServeAd(MaybeServeNewTabPageAdCallback callback) {
  NotifyFailedToServeNewTabPageAd();

  std::move(callback).Run(/*ads*/ absl::nullopt);
}

void Serving::NotifyOpportunityAroseToServeNewTabPageAd(
    const SegmentList& segments) const {
  for (ServingObserver& observer : observers_) {
    observer.OnOpportunityAroseToServeNewTabPageAd(segments);
  }
}

void Serving::NotifyDidServeNewTabPageAd(const NewTabPageAdInfo& ad) const {
  for (ServingObserver& observer : observers_) {
    observer.OnDidServeNewTabPageAd(ad);
  }
}

void Serving::NotifyFailedToServeNewTabPageAd() const {
  for (ServingObserver& observer : observers_) {
    observer.OnFailedToServeNewTabPageAd();
  }
}

}  // namespace ads::new_tab_page_ads

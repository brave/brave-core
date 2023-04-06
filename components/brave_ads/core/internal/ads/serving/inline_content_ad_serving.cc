/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving.h"

#include <utility>

#include "base/check.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_factory.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/inline_content_ads/inline_content_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

namespace brave_ads::inline_content_ads {

Serving::Serving(geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = features::GetServingVersion();
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

void Serving::MaybeServeAd(const std::string& dimensions,
                           MaybeServeInlineContentAdCallback callback) {
  if (!features::IsServingEnabled()) {
    BLOG(1, "Inline content ad not served: Feature is disabled");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  if (!IsSupported()) {
    BLOG(1, "Inline content ad not served: Unsupported version");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  if (!PermissionRules::HasPermission()) {
    BLOG(1,
         "Inline content ad not served: Not allowed due to permission rules");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  const targeting::UserModelInfo user_model = targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, dimensions,
      base::BindOnce(&Serving::OnGetForUserModel, weak_factory_.GetWeakPtr(),
                     user_model, dimensions, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void Serving::OnGetForUserModel(
    const targeting::UserModelInfo& user_model,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const bool had_opportunity,
    const CreativeInlineContentAdList& creative_ads) {
  if (had_opportunity) {
    const SegmentList segments = targeting::GetTopChildSegments(user_model);
    NotifyOpportunityAroseToServeInlineContentAd(segments);
  }

  if (creative_ads.empty()) {
    BLOG(1, "Inline content ad not served: No eligible ads found");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, static_cast<int>(creative_ads.size()) - 1);
  const CreativeInlineContentAdInfo& creative_ad = creative_ads.at(rand);

  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);
  ServeAd(ad, std::move(callback));
}

void Serving::ServeAd(const InlineContentAdInfo& ad,
                      MaybeServeInlineContentAdCallback callback) {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve inline content ad");
    return FailedToServeAd(ad.dimensions, std::move(callback));
  }

  BLOG(1, "Served inline content ad:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  title: " << ad.title << "\n"
              << "  description: " << ad.description << "\n"
              << "  imageUrl: " << ad.image_url << "\n"
              << "  dimensions: " << ad.dimensions << "\n"
              << "  ctaText: " << ad.cta_text << "\n"
              << "  targetUrl: " << ad.target_url);

  DCHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  NotifyDidServeInlineContentAd(ad);

  std::move(callback).Run(ad.dimensions, ad);
}

void Serving::FailedToServeAd(const std::string& dimensions,
                              MaybeServeInlineContentAdCallback callback) {
  NotifyFailedToServeInlineContentAd();

  std::move(callback).Run(dimensions, absl::nullopt);
}

void Serving::NotifyOpportunityAroseToServeInlineContentAd(
    const SegmentList& segments) const {
  for (ServingObserver& observer : observers_) {
    observer.OnOpportunityAroseToServeInlineContentAd(segments);
  }
}

void Serving::NotifyDidServeInlineContentAd(
    const InlineContentAdInfo& ad) const {
  for (ServingObserver& observer : observers_) {
    observer.OnDidServeInlineContentAd(ad);
  }
}

void Serving::NotifyFailedToServeInlineContentAd() const {
  for (ServingObserver& observer : observers_) {
    observer.OnFailedToServeInlineContentAd();
  }
}

}  // namespace brave_ads::inline_content_ads

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_factory.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/inline_content_ads/inline_content_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

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
  if (!IsInlineContentAdServingFeatureEnabled()) {
    BLOG(1, "Inline content ad not served: Feature is disabled");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  if (!IsSupported()) {
    BLOG(1, "Inline content ad not served: Unsupported version");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  if (!InlineContentAdPermissionRules::HasPermission()) {
    BLOG(1,
         "Inline content ad not served: Not allowed due to permission rules");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  BuildUserModel(base::BindOnce(&InlineContentAdServing::OnBuildUserModel,
                                weak_factory_.GetWeakPtr(), dimensions,
                                std::move(callback)));
}

void InlineContentAdServing::OnBuildUserModel(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const UserModelInfo& user_model) {
  CHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, dimensions,
      base::BindOnce(&InlineContentAdServing::OnGetForUserModel,
                     weak_factory_.GetWeakPtr(), user_model, dimensions,
                     std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAdServing::OnGetForUserModel(
    const UserModelInfo& user_model,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const bool had_opportunity,
    const CreativeInlineContentAdList& creative_ads) {
  if (had_opportunity) {
    if (delegate_) {
      delegate_->OnOpportunityAroseToServeInlineContentAd(
          GetTopChildSegments(user_model));
    }
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

void InlineContentAdServing::ServeAd(
    const InlineContentAdInfo& ad,
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

  CHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  if (delegate_) {
    delegate_->OnDidServeInlineContentAd(ad);
  }

  std::move(callback).Run(ad.dimensions, ad);
}

void InlineContentAdServing::FailedToServeAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (delegate_) {
    delegate_->OnFailedToServeInlineContentAd();
  }

  std::move(callback).Run(dimensions, /*ad*/ absl::nullopt);
}

}  // namespace brave_ads

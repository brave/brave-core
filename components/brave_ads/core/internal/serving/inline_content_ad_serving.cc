/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"

#include <utility>

#include "base/functional/bind.h"
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
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"

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
    MaybeServeInlineContentAdCallback callback) const {
  const auto result = CanServeAd();
  if (!result.has_value()) {
    BLOG(1, result.error());
    return FailedToServeAd(dimensions, std::move(callback));
  }

  NotifyOpportunityAroseToServeInlineContentAd();

  GetEligibleAds(dimensions, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

base::expected<void, std::string> InlineContentAdServing::CanServeAd() const {
  if (!base::FeatureList::IsEnabled(kInlineContentAdServingFeature)) {
    return base::unexpected(
        "Inline content ad not served: Feature is disabled");
  }

  if (!IsSupported()) {
    return base::unexpected(
        "Inline content ad not served: Unsupported version");
  }

  if (!InlineContentAdPermissionRules::HasPermission()) {
    return base::unexpected(
        "Inline content ad not served: Not allowed due to permission rules");
  }

  return base::ok();
}

void InlineContentAdServing::GetEligibleAds(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) const {
  BuildUserModel(base::BindOnce(&InlineContentAdServing::BuildUserModelCallback,
                                weak_factory_.GetWeakPtr(), dimensions,
                                std::move(callback)));
}

void InlineContentAdServing::BuildUserModelCallback(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const UserModelInfo& user_model) const {
  CHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, dimensions,
      base::BindOnce(
          &InlineContentAdServing::GetEligibleAdsForUserModelCallback,
          weak_factory_.GetWeakPtr(), dimensions, std::move(callback)));
}

void InlineContentAdServing::GetEligibleAdsForUserModelCallback(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const CreativeInlineContentAdList& creative_ads) const {
  if (creative_ads.empty()) {
    BLOG(1, "Inline content ad not served: No eligible ads found");
    return FailedToServeAd(dimensions, std::move(callback));
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  ServeAd(BuildInlineContentAd(ChooseCreativeAd(creative_ads)),
          std::move(callback));
}

void InlineContentAdServing::ServeAd(
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback) const {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve inline content ad");
    return FailedToServeAd(ad.dimensions, std::move(callback));
  }

  CHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  SuccessfullyServedAd(ad, std::move(callback));
}

void InlineContentAdServing::SuccessfullyServedAd(
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback) const {
  NotifyDidServeInlineContentAd(ad);

  std::move(callback).Run(ad.dimensions, ad);
}

void InlineContentAdServing::FailedToServeAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) const {
  NotifyFailedToServeInlineContentAd();

  std::move(callback).Run(dimensions, /*ad*/ absl::nullopt);
}

void InlineContentAdServing::NotifyOpportunityAroseToServeInlineContentAd()
    const {
  if (delegate_) {
    delegate_->OnOpportunityAroseToServeInlineContentAd();
  }
}

void InlineContentAdServing::NotifyDidServeInlineContentAd(
    const InlineContentAdInfo& ad) const {
  if (delegate_) {
    delegate_->OnDidServeInlineContentAd(ad);
  }
}

void InlineContentAdServing::NotifyFailedToServeInlineContentAd() const {
  if (delegate_) {
    delegate_->OnFailedToServeInlineContentAd();
  }
}

}  // namespace brave_ads

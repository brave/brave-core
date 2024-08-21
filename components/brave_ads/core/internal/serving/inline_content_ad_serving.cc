/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"

#include <optional>
#include <utility>

#include "base/debug/dump_without_crashing.h"
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
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

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
  const auto result = CanServeAd();
  if (!result.has_value()) {
    BLOG(1, result.error());
    return FailedToServeAd(dimensions, std::move(callback));
  }

  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetVisible();
  if (!tab) {
    return FailedToServeAd(dimensions, std::move(callback));
  }

  NotifyOpportunityAroseToServeInlineContentAd();

  GetUserModel(tab->id, dimensions, std::move(callback));
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

void InlineContentAdServing::GetUserModel(
    const int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  BuildUserModel(base::BindOnce(&InlineContentAdServing::GetEligibleAds,
                                weak_factory_.GetWeakPtr(), tab_id, dimensions,
                                std::move(callback)));
}

void InlineContentAdServing::GetEligibleAds(
    const int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    UserModelInfo user_model) const {
  eligible_ads_->GetForUserModel(
      std::move(user_model), dimensions,
      base::BindOnce(&InlineContentAdServing::GetEligibleAdsCallback,
                     weak_factory_.GetWeakPtr(), tab_id, dimensions,
                     std::move(callback)));
}

void InlineContentAdServing::GetEligibleAdsCallback(
    const int32_t tab_id,
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback,
    const CreativeInlineContentAdList& creative_ads) const {
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
    const int32_t tab_id,
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback) const {
  if (!ad.IsValid()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid inline content ad");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to serve inline content ad due to the ad being invalid");

    return FailedToServeAd(ad.dimensions, std::move(callback));
  }

  eligible_ads_->SetLastServedAd(ad);

  SuccessfullyServedAd(tab_id, ad, std::move(callback));
}

void InlineContentAdServing::SuccessfullyServedAd(
    const int32_t tab_id,
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
    const int32_t tab_id,
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

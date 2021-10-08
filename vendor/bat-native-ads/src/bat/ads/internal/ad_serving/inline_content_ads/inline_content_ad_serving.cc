/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving.h"

#include "base/check.h"
#include "base/rand_util.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_permission_rules.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_base.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_factory.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/features/inline_content_ads/inline_content_ads_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace inline_content_ads {

AdServing::AdServing(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = ::ads::features::GetAdServingVersion();
  eligible_ads_ = EligibleAdsFactory::Build(version, subdivision_targeting,
                                            anti_targeting_resource);
}

AdServing::~AdServing() = default;

void AdServing::AddObserver(InlineContentAdServingObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdServing::RemoveObserver(InlineContentAdServingObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdServing::MaybeServeAd(const std::string& dimensions,
                             GetInlineContentAdCallback callback) {
  if (!features::IsEnabled()) {
    BLOG(1, "Inline content ad not served: Feature is disabled");
    FailedToServeAd(dimensions, callback);
    return;
  }

  if (!IsSupported()) {
    BLOG(1, "Inline content ad not served: Unsupported version");
    FailedToServeAd(dimensions, callback);
    return;
  }

  frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1,
         "Inline content ad not served: Not allowed due to permission rules");
    FailedToServeAd(dimensions, callback);
    return;
  }

  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, dimensions,
      [=](const bool had_opportunity,
          const CreativeInlineContentAdList& creative_ads) {
        if (creative_ads.empty()) {
          BLOG(1, "Inline content ad not served: No eligible ads found");
          FailedToServeAd(dimensions, callback);
          return;
        }

        BLOG(1, "Found " << creative_ads.size() << " eligible ads");

        const int rand = base::RandInt(0, creative_ads.size() - 1);
        const CreativeInlineContentAdInfo creative_ad = creative_ads.at(rand);

        const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);
        if (!ServeAd(ad, callback)) {
          BLOG(1, "Failed to serve inline content ad");
          FailedToServeAd(dimensions, callback);
          return;
        }

        BLOG(1, "Served inline content ad");
        ServedAd(ad);
      });
}

///////////////////////////////////////////////////////////////////////////////

bool AdServing::IsSupported() const {
  if (!eligible_ads_) {
    return false;
  }

  return true;
}

bool AdServing::ServeAd(const InlineContentAdInfo& ad,
                        GetInlineContentAdCallback callback) const {
  DCHECK(ad.IsValid());

  BLOG(1, "Serving inline content ad:\n"
              << "  uuid: " << ad.uuid << "\n"
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

  callback(/* success */ true, ad.dimensions, ad);

  NotifyDidServeInlineContentAd(ad);

  return true;
}

void AdServing::FailedToServeAd(const std::string& dimensions,
                                GetInlineContentAdCallback callback) {
  callback(/* success */ false, dimensions, {});

  NotifyFailedToServeInlineContentAd();
}

void AdServing::ServedAd(const InlineContentAdInfo& ad) {
  DCHECK(eligible_ads_);
  eligible_ads_->set_last_served_ad(ad);
}

void AdServing::NotifyDidServeInlineContentAd(
    const InlineContentAdInfo& ad) const {
  for (InlineContentAdServingObserver& observer : observers_) {
    observer.OnDidServeInlineContentAd(ad);
  }
}

void AdServing::NotifyFailedToServeInlineContentAd() const {
  for (InlineContentAdServingObserver& observer : observers_) {
    observer.OnFailedToServeInlineContentAd();
  }
}

}  // namespace inline_content_ads
}  // namespace ads

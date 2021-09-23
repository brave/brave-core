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
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/features/inline_content_ads/inline_content_ads_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace inline_content_ads {

AdServing::AdServing(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : eligible_ads_(std::make_unique<EligibleAds>(subdivision_targeting,
                                                  anti_targeting_resource)) {}

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
  InlineContentAdInfo inline_content_ad;

  if (!features::inline_content_ads::IsEnabled()) {
    callback(/* success */ false, dimensions, inline_content_ad);
    return;
  }

  frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1,
         "Inline content ad not served: Not allowed due to permission rules");
    NotifyFailedToServeInlineContentAd();
    callback(/* success */ false, dimensions, inline_content_ad);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/17542): Refactor Brave
  // Ads serving
  const int ad_serving_version = features::GetAdServingVersion();
  BLOG(1, "Ad serving version " << ad_serving_version);

  switch (ad_serving_version) {
    case 1: {
      MaybeServeAdV1(dimensions, callback);
      break;
    }

    case 2: {
      MaybeServeAdV2(dimensions, callback);
      break;
    }

    default: {
      NOTREACHED() << "Ad serving version is not supported";
      break;
    }
  }
}

void AdServing::MaybeServeAdV1(const std::string& dimensions,
                               GetInlineContentAdCallback callback) {
  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  eligible_ads_->Get(
      user_model, dimensions,
      [=](const bool was_allowed, const CreativeInlineContentAdList& ads) {
        InlineContentAdInfo inline_content_ad;

        if (ads.empty()) {
          BLOG(1, "Inline content ad not served: No eligible ads found");
          NotifyFailedToServeInlineContentAd();
          callback(/* success */ false, dimensions, inline_content_ad);
          return;
        }

        BLOG(1, "Found " << ads.size() << " eligible ads");

        const int rand = base::RandInt(0, ads.size() - 1);
        const CreativeInlineContentAdInfo ad = ads.at(rand);

        eligible_ads_->SetLastServedAd(ad);

        inline_content_ad = BuildInlineContentAd(ad);

        BLOG(1, "Serving inline content ad:\n"
                    << "  uuid: " << inline_content_ad.uuid << "\n"
                    << "  creativeInstanceId: "
                    << inline_content_ad.creative_instance_id << "\n"
                    << "  creativeSetId: " << inline_content_ad.creative_set_id
                    << "\n"
                    << "  campaignId: " << inline_content_ad.campaign_id << "\n"
                    << "  advertiserId: " << inline_content_ad.advertiser_id
                    << "\n"
                    << "  segment: " << inline_content_ad.segment << "\n"
                    << "  title: " << inline_content_ad.title << "\n"
                    << "  description: " << inline_content_ad.description
                    << "\n"
                    << "  imageUrl: " << inline_content_ad.image_url << "\n"
                    << "  dimensions: " << inline_content_ad.dimensions << "\n"
                    << "  ctaText: " << inline_content_ad.cta_text << "\n"
                    << "  targetUrl: " << inline_content_ad.target_url);

        NotifyDidServeInlineContentAd(inline_content_ad);

        callback(/* success */ true, dimensions, inline_content_ad);
      });
}

void AdServing::MaybeServeAdV2(const std::string& dimensions,
                               GetInlineContentAdCallback callback) {
  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  eligible_ads_->GetV2(
      user_model, dimensions,
      [=](const bool was_allowed,
          const absl::optional<CreativeInlineContentAdInfo>& ad) {
        InlineContentAdInfo inline_content_ad;

        if (!ad) {
          BLOG(1, "Inline content ad not served: No eligible ads found");
          NotifyFailedToServeInlineContentAd();
          callback(/* success */ false, dimensions, inline_content_ad);
          return;
        }

        eligible_ads_->SetLastServedAd(ad.value());

        inline_content_ad = BuildInlineContentAd(ad.value());

        BLOG(1, "Serving inline content ad:\n"
                    << "  uuid: " << inline_content_ad.uuid << "\n"
                    << "  creativeInstanceId: "
                    << inline_content_ad.creative_instance_id << "\n"
                    << "  creativeSetId: " << inline_content_ad.creative_set_id
                    << "\n"
                    << "  campaignId: " << inline_content_ad.campaign_id << "\n"
                    << "  advertiserId: " << inline_content_ad.advertiser_id
                    << "\n"
                    << "  segment: " << inline_content_ad.segment << "\n"
                    << "  title: " << inline_content_ad.title << "\n"
                    << "  description: " << inline_content_ad.description
                    << "\n"
                    << "  imageUrl: " << inline_content_ad.image_url << "\n"
                    << "  dimensions: " << inline_content_ad.dimensions << "\n"
                    << "  ctaText: " << inline_content_ad.cta_text << "\n"
                    << "  targetUrl: " << inline_content_ad.target_url);

        NotifyDidServeInlineContentAd(inline_content_ad);

        callback(/* success */ true, dimensions, inline_content_ad);
      });
}

///////////////////////////////////////////////////////////////////////////////

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

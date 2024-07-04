/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CONSTANTS_H_

namespace brave_ads {

inline constexpr char kCreativeSearchResultAdsProductMojomEntityType[] =
    "Product";

inline constexpr char kCreativeSearchResultAdsMojomPropertyName[] = "creatives";

inline constexpr char kCreativeSearchResultAdMojomEntityType[] =
    "SearchResultAd";

// Creative ad.
inline constexpr char kCreativeAdPlacementIdPropertyName[] =
    "data-placement-id";
inline constexpr char kCreativeAdCreativeInstanceIdPropertyName[] =
    "data-creative-instance-id";
inline constexpr char kCreativeAdCreativeSetIdPropertyName[] =
    "data-creative-set-id";
inline constexpr char kCreativeAdCampaignIdPropertyName[] = "data-campaign-id";
inline constexpr char kCreativeAdAdvertiserIdPropertyName[] =
    "data-advertiser-id";
inline constexpr char kCreativeAdLandingPagePropertyName[] =
    "data-landing-page";
inline constexpr char kCreativeAdHeadlineTextPropertyName[] =
    "data-headline-text";
inline constexpr char kCreativeAdDescriptionPropertyName[] = "data-description";
inline constexpr char kCreativeAdRewardsValuePropertyName[] =
    "data-rewards-value";

// Creative set conversion.
inline constexpr char kCreativeSetConversionUrlPatternPropertyName[] =
    "data-conversion-url-pattern-value";
inline constexpr char kCreativeSetConversionAdvertiserPublicKeyPropertyName[] =
    "data-conversion-advertiser-public-key-value";
inline constexpr char kCreativeSetConversionObservationWindowPropertyName[] =
    "data-conversion-observation-window-value";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREATIVES_SEARCH_RESULT_AD_CREATIVE_SEARCH_RESULT_AD_CONSTANTS_H_

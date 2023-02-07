/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/conversions/conversions_database_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_resource.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

base::Time CalculateExpireAtTime(const int observation_window) {
  return Now() + base::Days(observation_window);
}

}  // namespace

class BatAdsConversionsTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    conversions_ = std::make_unique<Conversions>();
    ad_events_database_table_ = std::make_unique<database::table::AdEvents>();
    conversion_queue_database_table_ =
        std::make_unique<database::table::ConversionQueue>();
    conversions_database_table_ =
        std::make_unique<database::table::Conversions>();
  }

  std::unique_ptr<Conversions> conversions_;
  std::unique_ptr<database::table::AdEvents> ad_events_database_table_;
  std::unique_ptr<database::table::ConversionQueue>
      conversion_queue_database_table_;
  std::unique_ptr<database::table::Conversions> conversions_database_table_;
};

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedNotificationAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertViewedNotificationAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertClickedNotificationAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertClickedNotificationAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedNewTabPageAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                            ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertViewedNewTabPageAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                            ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertClickedNewTabPageAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertClickedNewTabPageAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kNewTabPageAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedPromotedContentAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertViewedPromotedContentAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertClickedPromotedContentAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest,
       ConvertClickedPromotedContentAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(creative_ad, AdType::kPromotedContentAd,
                   ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedInlineContentAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedInlineContentAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertClickedInlineContentAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*/baz";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar/baz")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, ConvertClickedInlineContentAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kInlineContentAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*/baz";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar/baz")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertViewedSearchResultAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertViewedSearchResultAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertClickedSearchResultAdWhenAdsAreDisabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertClickedSearchResultAdWhenAdsAreEnabled) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event_1 = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kClicked, Now());
  FireAdEvent(ad_event_2);

  ConversionList conversions;
  ConversionInfo conversion;
  conversion.creative_set_id = creative_ad.creative_set_id;
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);
  database::SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());

                       const AdEventInfo& ad_event = ad_events.front();
                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, ConvertMultipleAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.foo.com/*";
  conversion_1.observation_window = 3;
  conversion_1.expire_at =
      CalculateExpireAtTime(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;
  conversion_2.creative_set_id = "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.foo.com/*/baz";
  conversion_2.observation_window = 3;
  conversion_2.expire_at =
      CalculateExpireAtTime(conversion_2.observation_window);
  conversions.push_back(conversion_2);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event_1 =
      BuildAdEvent("7ee858e8-6306-4317-88c3-9e7d58afad26",
                   conversion_1.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event_1);

  AdvanceClockBy(base::Minutes(1));

  const AdEventInfo ad_event_2 =
      BuildAdEvent("da2d3397-bc97-46d1-a323-d8723c0a6b33",
                   conversion_2.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event_2);
  const AdEventInfo ad_event_3 =
      BuildAdEvent("da2d3397-bc97-46d1-a323-d8723c0a6b33",
                   conversion_2.creative_set_id, ConfirmationType::kClicked);
  FireAdEvent(ad_event_3);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/qux")}, {}, {});

  conversions_->MaybeConvert({GURL("https://www.foo.com/bar/baz")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "(creative_set_id = '%s' OR creative_set_id = '%s') AND "
      "confirmation_type = 'conversion'",
      conversion_1.creative_set_id.c_str(),
      conversion_2.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce(
          [](const ConversionList& conversions, const bool success,
             const AdEventList& ad_events) {
            ASSERT_TRUE(success);

            EXPECT_EQ(2UL, ad_events.size());

            const ConversionInfo& conversion_1 = conversions.at(0);
            const AdEventInfo& ad_event_1 = ad_events.at(1);
            EXPECT_EQ(conversion_1.creative_set_id, ad_event_1.creative_set_id);

            const ConversionInfo& conversion_2 = conversions.at(1);
            const AdEventInfo& ad_event_2 = ad_events.at(0);
            EXPECT_EQ(conversion_2.creative_set_id, ad_event_2.creative_set_id);
          },
          std::move(conversions)));
}

TEST_F(BatAdsConversionsTest, ConvertViewedAdWhenAdWasDismissed) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*bar*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);
  FireAdEvent(ad_event_2);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/quxbarbaz")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/bar";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event_1 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);
  FireAdEvent(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kServed);
  FireAdEvent(ad_event_2);
  const AdEventInfo ad_event_3 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kTransferred);
  FireAdEvent(ad_event_3);
  const AdEventInfo ad_event_4 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kFlagged);
  FireAdEvent(ad_event_4);
  const AdEventInfo ad_event_5 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kSaved);
  FireAdEvent(ad_event_5);
  const AdEventInfo ad_event_6 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kUpvoted);
  FireAdEvent(ad_event_6);
  const AdEventInfo ad_event_7 =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kDownvoted);
  FireAdEvent(ad_event_7);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, DoNotConvertViewedAdForPostClick) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/bar";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, DoNotConvertAdIfConversionDoesNotExist) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  const AdEventInfo ad_event =
      BuildAdEvent(creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition =
      "creative_set_id = 'foobar' AND "
      "confirmation_type = 'conversion'";

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertAdWhenThereIsConversionHistoryForTheSameCreativeSet) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest,
       DoNotConvertAdWhenUrlDoesNotMatchConversionIdPattern) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/bar/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/qux")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertAdWhenTheConversionIsOnTheCuspOfExpiring) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://*.bar.com/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  AdvanceClockBy(base::Days(3) - base::Seconds(1));

  // Act
  conversions_->MaybeConvert({GURL("https://foo.bar.com/qux")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, DoNotConvertAdWhenTheConversionHasExpired) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/b*r/*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  AdvanceClockBy(base::Days(3));

  // Act
  conversions_->MaybeConvert({GURL("https://www.foo.com/bar/qux")}, {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      }));
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainIntermediateUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/baz";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert(
      {GURL("https://foo.com/bar"), GURL("https://foo.com/baz"),
       GURL("https://foo.com/qux")},
      {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainOriginalUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/bar";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert(
      {GURL("https://foo.com/bar"), GURL("https://foo.com/baz"),
       GURL("https://foo.com/qux")},
      {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/qux";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert(
      {GURL("https://foo.com/bar"), GURL("https://foo.com/baz"),
       GURL("https://foo.com/qux")},
      {}, {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, base::BindOnce(
                     [](const ConversionInfo& conversion, const bool success,
                        const AdEventList& ad_events) {
                       ASSERT_TRUE(success);

                       EXPECT_EQ(1UL, ad_events.size());
                       const AdEventInfo& ad_event = ad_events.front();

                       EXPECT_EQ(conversion.creative_set_id,
                                 ad_event.creative_set_id);
                     },
                     std::move(conversion)));
}

TEST_F(BatAdsConversionsTest, ExtractConversionId) {
  // Arrange
  resource::Conversions resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/thankyou";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  conversions_->MaybeConvert(
      {GURL("https://foo.bar/"), GURL("https://brave.com/thankyou")},
      R"(<html><meta name="ad-conversion-id" content="abc123"></html>)",
      resource.get()->id_patterns);

  // Assert
  conversion_queue_database_table_->GetAll(base::BindOnce(
      [](const ConversionInfo& conversion, const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        const ConversionQueueItemInfo& conversion_queue_item =
            conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id,
                  conversion_queue_item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key,
                  conversion_queue_item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, conversion_queue_item.conversion_id);
      },
      conversion));
}

TEST_F(BatAdsConversionsTest, ExtractConversionIdWithResourcePatternFromHtml) {
  // Arrange
  resource::Conversions resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/foobar";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  // See associated patterns in the verifiable conversion resource
  // /data/test/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb
  conversions_->MaybeConvert(
      {GURL("https://foo.bar/"), GURL("https://brave.com/foobar")},
      "<html><div id=\"conversion-id\">abc123</div></html>",
      resource.get()->id_patterns);

  // Assert
  conversion_queue_database_table_->GetAll(base::BindOnce(
      [](const ConversionInfo& conversion, const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        const ConversionQueueItemInfo& conversion_queue_item =
            conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id,
                  conversion_queue_item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key,
                  conversion_queue_item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, conversion_queue_item.conversion_id);
      },
      conversion));
}

TEST_F(BatAdsConversionsTest, ExtractConversionIdWithResourcePatternFromUrl) {
  // Arrange
  resource::Conversions resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/foobar?conversion_id=*";
  conversion.observation_window = 3;
  conversion.expire_at = CalculateExpireAtTime(conversion.observation_window);
  conversions.push_back(conversion);

  database::SaveConversions(conversions);

  const AdEventInfo ad_event =
      BuildAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(ad_event);

  // Act
  // See associated patterns in the verifiable conversion resource
  // /data/test/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb
  conversions_->MaybeConvert(
      {GURL("https://foo.bar/"),
       GURL("https://brave.com/foobar?conversion_id=abc123")},
      "<html><div id=\"conversion-id\">foobar</div></html>",
      resource.get()->id_patterns);

  // Assert
  conversion_queue_database_table_->GetAll(base::BindOnce(
      [](const ConversionInfo& conversion, const bool success,
         const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        const ConversionQueueItemInfo& conversion_queue_item =
            conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id,
                  conversion_queue_item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key,
                  conversion_queue_item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, conversion_queue_item.conversion_id);
      },
      conversion));
}

}  // namespace ads

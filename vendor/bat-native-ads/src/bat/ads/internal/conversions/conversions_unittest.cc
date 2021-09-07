/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions.h"

#include <cstdint>
#include <memory>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/database/tables/conversions_database_table.h"
#include "bat/ads/internal/resources/conversions/conversions_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConversionsTest : public UnitTestBase {
 protected:
  BatAdsConversionsTest()
      : conversions_(std::make_unique<Conversions>()),
        ad_events_database_table_(
            std::make_unique<database::table::AdEvents>()),
        conversion_queue_database_table_(
            std::make_unique<database::table::ConversionQueue>()),
        conversions_database_table_(
            std::make_unique<database::table::Conversions>()) {}

  ~BatAdsConversionsTest() override = default;

  void SaveConversions(const ConversionList& conversions) {
    conversions_database_table_->Save(
        conversions, [](const bool success) { ASSERT_TRUE(success); });
  }

  int64_t CalculateExpiryTimestamp(const int observation_window) {
    base::Time time = base::Time::Now();
    time += base::TimeDelta::FromDays(observation_window);

    return static_cast<int64_t>(time.ToDoubleT());
  }

  void FireAdEvent(const std::string& creative_set_id,
                   const ConfirmationType confirmation_type) {
    AdEventInfo ad_event;
    ad_event.type = AdType::kAdNotification;
    ad_event.creative_instance_id = "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
    ad_event.creative_set_id = creative_set_id;
    ad_event.timestamp = NowAsTimestamp();
    ad_event.confirmation_type = confirmation_type;

    LogAdEvent(ad_event, [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<Conversions> conversions_;
  std::unique_ptr<database::table::AdEvents> ad_events_database_table_;
  std::unique_ptr<database::table::ConversionQueue>
      conversion_queue_database_table_;
  std::unique_ptr<database::table::Conversions> conversions_database_table_;
};

TEST_F(BatAdsConversionsTest, ShouldNotAllowConversionTracking) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowConversionTracking,
                                   false);

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foobar.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  // Act
  conversions_->MaybeConvert({"https://www.foobar.com/signup"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
}

TEST_F(BatAdsConversionsTest, ConvertViewedAd) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ConvertClickedAd) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/*/baz";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kClicked);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar/baz"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ConvertMultipleAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion_1;
  conversion_1.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion_1.type = "postview";
  conversion_1.url_pattern = "https://www.foo.com/*";
  conversion_1.observation_window = 3;
  conversion_1.expiry_timestamp =
      CalculateExpiryTimestamp(conversion_1.observation_window);
  conversions.push_back(conversion_1);

  ConversionInfo conversion_2;
  conversion_2.creative_set_id = "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8";
  conversion_2.type = "postclick";
  conversion_2.url_pattern = "https://www.foo.com/*/baz";
  conversion_2.observation_window = 3;
  conversion_2.expiry_timestamp =
      CalculateExpiryTimestamp(conversion_2.observation_window);
  conversions.push_back(conversion_2);

  SaveConversions(conversions);

  FireAdEvent(conversion_1.creative_set_id, ConfirmationType::kViewed);

  FireAdEvent(conversion_2.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(conversion_2.creative_set_id, ConfirmationType::kClicked);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/qux"}, "", {});

  conversions_->MaybeConvert({"https://www.foo.com/bar/baz"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "(creative_set_id = '%s' OR creative_set_id = '%s') AND "
      "confirmation_type = 'conversion'",
      conversion_1.creative_set_id.c_str(),
      conversion_2.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversions](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(2UL, ad_events.size());

        const ConversionInfo conversion_1 = conversions.at(0);
        const AdEventInfo ad_event_1 = ad_events.at(0);
        EXPECT_EQ(conversion_1.creative_set_id, ad_event_1.creative_set_id);

        const ConversionInfo conversion_2 = conversions.at(1);
        const AdEventInfo ad_event_2 = ad_events.at(1);
        EXPECT_EQ(conversion_2.creative_set_id, ad_event_2.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ConvertViewedAdWhenAdWasDismissed) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/*bar*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/quxbarbaz"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/bar";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kDismissed);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kTransferred);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kFlagged);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kUpvoted);
  FireAdEvent(conversion.creative_set_id, ConfirmationType::kDownvoted);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
}

TEST_F(BatAdsConversionsTest, DoNotConvertViewedAdForPostClick) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postclick";
  conversion.url_pattern = "https://www.foo.com/bar";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
}

TEST_F(BatAdsConversionsTest, DoNotConvertAdIfConversionDoesNotExist) {
  // Arrange
  const std::string creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

  FireAdEvent(creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Assert
  const std::string condition =
      "creative_set_id = 'foobar' AND "
      "confirmation_type = 'conversion'";

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
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
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
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
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/qux"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
}

TEST_F(BatAdsConversionsTest, ConvertAdWhenTheConversionIsOnTheCuspOfExpiring) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://*.bar.com/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(3) -
                                  base::TimeDelta::FromMinutes(1));

  // Act
  conversions_->MaybeConvert({"https://foo.bar.com/qux"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, DoNotConvertAdWhenTheConversionHasExpired) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://www.foo.com/b*r/*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  task_environment_.FastForwardBy(base::TimeDelta::FromDays(3));

  // Act
  conversions_->MaybeConvert({"https://www.foo.com/bar/qux"}, "", {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition, [](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(ad_events.empty());
      });
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainIntermediateUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/baz";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert(
      {"https://foo.com/bar", "https://foo.com/baz", "https://foo.com/qux"}, "",
      {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainOriginalUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/bar";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert(
      {"https://foo.com/bar", "https://foo.com/baz", "https://foo.com/qux"}, "",
      {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ConvertAdForRedirectChainUrl) {
  // Arrange
  ConversionList conversions;

  ConversionInfo conversion;
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://foo.com/qux";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert(
      {"https://foo.com/bar", "https://foo.com/baz", "https://foo.com/qux"}, "",
      {});

  // Assert
  const std::string condition = base::StringPrintf(
      "creative_set_id = '%s' AND confirmation_type = 'conversion'",
      conversion.creative_set_id.c_str());

  ad_events_database_table_->GetIf(
      condition,
      [&conversion](const bool success, const AdEventList& ad_events) {
        ASSERT_TRUE(success);

        EXPECT_EQ(1UL, ad_events.size());
        AdEventInfo ad_event = ad_events.front();

        EXPECT_EQ(conversion.creative_set_id, ad_event.creative_set_id);
      });
}

TEST_F(BatAdsConversionsTest, ExtractConversionId) {
  // Arrange
  resource::Conversions resource;
  resource.Load();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/thankyou";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  conversions_->MaybeConvert(
      {"https://foo.bar/", "https://brave.com/thankyou"},
      "<html><meta name=\"ad-conversion-id\" content=\"abc123\"></html>",
      resource.get());

  // Assert
  conversion_queue_database_table_->GetAll(
      [=](const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        ConversionQueueItemInfo item = conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id, item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key, item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, item.conversion_id);
      });
}

TEST_F(BatAdsConversionsTest, ExtractConversionIdWithResourcePatternFromHtml) {
  // Arrange
  resource::Conversions resource;
  resource.Load();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/foobar";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  // See associated patterns in the verifiable conversion resource
  // /data/test/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb
  conversions_->MaybeConvert(
      {"https://foo.bar/", "https://brave.com/foobar"},
      "<html><div id=\"conversion-id\">abc123</div></html>", resource.get());

  // Assert
  conversion_queue_database_table_->GetAll(
      [=](const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        ConversionQueueItemInfo item = conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id, item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key, item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, item.conversion_id);
      });
}

TEST_F(BatAdsConversionsTest, ExtractConversionIdWithResourcePatternFromUrl) {
  // Arrange
  resource::Conversions resource;
  resource.Load();

  ConversionList conversions;

  ConversionInfo conversion;
  conversion.advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion.creative_set_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  conversion.type = "postview";
  conversion.url_pattern = "https://brave.com/foobar?conversion_id=*";
  conversion.observation_window = 3;
  conversion.expiry_timestamp =
      CalculateExpiryTimestamp(conversion.observation_window);
  conversions.push_back(conversion);

  SaveConversions(conversions);

  FireAdEvent(conversion.creative_set_id, ConfirmationType::kViewed);

  // Act
  // See associated patterns in the verifiable conversion resource
  // /data/test/resources/nnqccijfhvzwyrxpxwjrpmynaiazctqb
  conversions_->MaybeConvert(
      {"https://foo.bar/", "https://brave.com/foobar?conversion_id=abc123"},
      "<html><div id=\"conversion-id\">foobar</div></html>", resource.get());

  // Assert
  conversion_queue_database_table_->GetAll(
      [=](const bool success,
          const ConversionQueueItemList& conversion_queue_items) {
        ASSERT_TRUE(success);

        ASSERT_EQ(1UL, conversion_queue_items.size());
        ConversionQueueItemInfo item = conversion_queue_items.front();

        ASSERT_EQ(conversion.creative_set_id, item.creative_set_id);
        ASSERT_EQ(conversion.advertiser_public_key, item.advertiser_public_key);

        const std::string expected_conversion_id = "abc123";
        EXPECT_EQ(expected_conversion_id, item.conversion_id);
      });
}

}  // namespace ads

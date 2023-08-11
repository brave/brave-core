/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildConversionUserDataForRewardsUser) {
  // Arrange
  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kViewed,
      /*is_verifiable*/ false, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        EXPECT_EQ(
            base::test::ParseJsonDict(R"({"conversion":[{"action":"view"}]})"),
            user_data);
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildVerifiableConversionUserDataForRewardsUser) {
  // Arrange
  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kClicked,
      /*is_verifiable*/ true, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));
        const std::string pattern =
            R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildConversionUserDataForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kViewed,
      /*is_verifiable*/ false, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        EXPECT_EQ(
            base::test::ParseJsonDict(R"({"conversion":[{"action":"view"}]})"),
            user_data);
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildVerifiableConversionUserDataForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kClicked,
      /*is_verifiable*/ true, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));
        const std::string pattern =
            R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionUserDataForMissingCreativeInstanceId) {
  // Arrange
  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kViewed,
      /*is_verifiable*/ false, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(kMissingCreativeInstanceId,
                          base::BindOnce([](base::Value::Dict user_data) {
                            // Assert
                            EXPECT_TRUE(user_data.empty());
                          }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildVerifiableConversionUserDataForMissingCreativeInstanceId) {
  // Arrange
  BuildAndSaveConversionQueueItemsForTesting(
      AdType::kNotificationAd, ConfirmationType::kClicked,
      /*is_verifiable*/ true, /*should_use_random_uuids*/ false, /*count*/ 1);

  // Act
  BuildConversionUserData(kMissingCreativeInstanceId,
                          base::BindOnce([](base::Value::Dict user_data) {
                            // Assert
                            EXPECT_TRUE(user_data.empty());
                          }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionUserDataIfQueueIsEmpty) {
  // Arrange

  // Act
  BuildConversionUserData(kCreativeInstanceId,
                          base::BindOnce([](base::Value::Dict user_data) {
                            // Assert
                            EXPECT_TRUE(user_data.empty());
                          }));
}

}  // namespace brave_ads

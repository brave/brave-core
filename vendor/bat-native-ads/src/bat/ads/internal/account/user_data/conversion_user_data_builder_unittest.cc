/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data_builder.h"

#include "absl/types/optional.h"
#include "base/test/values_test_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/conversions/conversion_queue_item_unittest_util.h"
#include "bat/ads/internal/conversions/verifiable_conversion_envelope_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {
namespace builder {

namespace {

constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kMissingCreativeInstanceId[] =
    "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";

constexpr char kConversionId[] = "smartbrownfoxes42";
constexpr char kEmptyConversionId[] = "";

constexpr char kAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
constexpr char kEmptyAdvertiserPublicKey[] = "";

constexpr char kAdvertiserSecretKey[] =
    "Ete7+aKfrX25gt0eN4kBV1LqeF9YmB1go8OqnGXUGG4=";

}  // namespace

class BatAdsConversionUserDataBuilderTest : public UnitTestBase {
 protected:
  BatAdsConversionUserDataBuilderTest() = default;

  ~BatAdsConversionUserDataBuilderTest() override = default;
};

TEST_F(BatAdsConversionUserDataBuilderTest, BuildConversion) {
  // Arrange
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Act
  BuildConversion(kCreativeInstanceId, [](base::Value::Dict user_data) {
    const absl::optional<std::string> message =
        security::OpenEvenlopeForUserDataAndAdvertiserSecretKey(
            user_data, kAdvertiserSecretKey);
    ASSERT_TRUE(message);

    const std::string expected_message = kConversionId;
    EXPECT_EQ(expected_message, *message);
  });

  // Assert
}

TEST_F(BatAdsConversionUserDataBuilderTest,
       DoNotBuildConversionForMissingCreativeInstanceId) {
  // Arrange
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Act
  BuildConversion(kMissingCreativeInstanceId, [](base::Value::Dict user_data) {
    // Assert
    const base::Value expected_user_data = base::test::ParseJson("{}");
    ASSERT_TRUE(expected_user_data.is_dict());

    EXPECT_EQ(expected_user_data, user_data);
  });

  // Assert
}

TEST_F(BatAdsConversionUserDataBuilderTest,
       DoNotBuildConversionIfConversionIdOrAdvertiserPublicKeyIsEmpty) {
  // Arrange
  BuildAndSaveConversionQueueItem(kEmptyConversionId,
                                  kEmptyAdvertiserPublicKey);

  // Act
  BuildConversion(kCreativeInstanceId, [](base::Value::Dict user_data) {
    // Assert
    const base::Value expected_user_data = base::test::ParseJson("{}");
    ASSERT_TRUE(expected_user_data.is_dict());

    EXPECT_EQ(expected_user_data, user_data);
  });

  // Assert
}

}  // namespace builder
}  // namespace user_data
}  // namespace ads

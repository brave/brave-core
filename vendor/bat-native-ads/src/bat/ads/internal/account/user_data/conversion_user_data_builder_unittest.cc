/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data_builder.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info_aliases.h"
#include "bat/ads/internal/conversions/conversion_queue_item_unittest_util.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

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
  user_data::BuildConversion(kCreativeInstanceId, [](base::Value user_data) {
    const absl::optional<std::string> message_optional =
        security::OpenEvenlopeForUserDataAndAdvertiserSecretKey(
            user_data, kAdvertiserSecretKey);
    ASSERT_TRUE(message_optional);
    const std::string message = message_optional.value();

    const std::string expected_message = kConversionId;

    EXPECT_EQ(expected_message, message);
  });

  // Assert
}

TEST_F(BatAdsConversionUserDataBuilderTest,
       DoNotBuildConversionForMissingCreativeInstanceId) {
  // Arrange
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Act
  user_data::BuildConversion(kMissingCreativeInstanceId,
                             [](base::Value user_data) {
                               std::string json;
                               base::JSONWriter::Write(user_data, &json);

                               const std::string expected_json = "{}";

                               EXPECT_EQ(expected_json, json);
                             });

  // Assert
}

TEST_F(BatAdsConversionUserDataBuilderTest,
       DoNotBuildConversionIfConversionIdOrAdvertiserPublicKeyIsEmpty) {
  // Arrange
  BuildAndSaveConversionQueueItem(kEmptyConversionId,
                                  kEmptyAdvertiserPublicKey);

  // Act
  user_data::BuildConversion(kCreativeInstanceId, [](base::Value user_data) {
    std::string json;
    base::JSONWriter::Write(user_data, &json);

    const std::string expected_json = "{}";

    EXPECT_EQ(expected_json, json);
  });

  // Assert
}

}  // namespace ads

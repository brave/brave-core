/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmation_user_data_builder.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/conversions/conversion_queue_item_unittest_util.h"
#include "bat/ads/sys_info.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCatalogId[] = "29e5c8bc0ba319069980bb390d8e8f9b58c05a20";

constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kConversionId[] = "smartbrownfoxes42";
constexpr char kAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";

}  // namespace

class BatAdsConfirmationUserDataBuilderTest : public UnitTestBase {};

TEST_F(BatAdsConfirmationUserDataBuilderTest,
       BuildForNonConversionConfirmationType) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  SysInfo().is_uncertain_future = false;

  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false);
  AdvanceClockTo(time);

  // Act
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Assert
  const ConfirmationUserDataBuilder user_data_builder(
      Now(), kCreativeInstanceId, ConfirmationType::kViewed);
  user_data_builder.Build(base::BindOnce([](base::Value::Dict user_data) {
    std::string json;
    ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

    const std::string pattern =
        R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","mutated":true,"odyssey":"host","platform":"windows","rotating_hash":"p3QDOuQ3HakWNXLBZCP8dktH\+zyu7FsHpKONKhWliJE=","studies":\[],"systemTimestamp":"2020-11-18T12:00:00.000Z","versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
    EXPECT_TRUE(RE2::FullMatch(json, pattern));
  }));
}

TEST_F(BatAdsConfirmationUserDataBuilderTest,
       BuildForConversionConfirmationType) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  SysInfo().is_uncertain_future = false;

  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false);
  AdvanceClockTo(time);

  // Act
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Assert
  const ConfirmationUserDataBuilder user_data_builder(
      Now(), kCreativeInstanceId, ConfirmationType::kConversion);
  user_data_builder.Build(base::BindOnce([](base::Value::Dict user_data) {
    std::string json;
    ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

    const std::string pattern =
        R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"conversionEnvelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":"(.{64})","epk":"(.{44})","nonce":"(.{32})"},"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","mutated":true,"odyssey":"host","platform":"windows","rotating_hash":"p3QDOuQ3HakWNXLBZCP8dktH\+zyu7FsHpKONKhWliJE=","studies":\[],"systemTimestamp":"2020-11-18T12:00:00.000Z","versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
    EXPECT_TRUE(RE2::FullMatch(json, pattern));
  }));
}

}  // namespace ads

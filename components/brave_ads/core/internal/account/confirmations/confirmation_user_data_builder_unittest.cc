/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildForNonConversionConfirmationType) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  TransactionInfo transaction =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kViewed);
  transaction.creative_instance_id = kCreativeInstanceId;

  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_guids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildConfirmationUserData(
      transaction, base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"(.{44})","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildForConversionConfirmationType) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value*/ 0.0, ConfirmationType::kConversion);
  transaction.creative_instance_id = kCreativeInstanceId;

  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_guids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildConfirmationUserData(
      transaction, base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"conversionEnvelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":"(.{64})","epk":"(.{44})","nonce":"(.{32})"},"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"(.{44})","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

}  // namespace brave_ads

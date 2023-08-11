/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/fixed/confirmation_fixed_user_data_builder.h"

#include <string>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsFixedUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsFixedUserDataBuilderTest, Build) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transaction.creative_instance_id = kCreativeInstanceId;

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Act

  // Assert
  BuildFixedUserData(
      transaction, base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"(.{44})","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsFixedUserDataBuilderTest, BuildForDefaultConversion) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  transaction.creative_instance_id = kCreativeInstanceId;

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt);
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Act

  // Assert
  BuildFixedUserData(
      transaction, base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"conversion":\[{"action":"view"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"(.{44})","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsFixedUserDataBuilderTest, BuildForVerifiableConversion) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SetCatalogId(kCatalogId);

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids*/ true);
  transaction.creative_instance_id = kCreativeInstanceId;

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  // Act

  // Assert
  BuildFixedUserData(
      transaction, base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"conversion":\[{"action":"view"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":"(.{64})","epk":"(.{44})","nonce":"(.{32})"}}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"(.{44})","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

}  // namespace brave_ads

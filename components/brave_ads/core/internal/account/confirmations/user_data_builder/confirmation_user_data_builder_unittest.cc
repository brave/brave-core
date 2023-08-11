/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"

#include <string>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/catalog_permission_rule_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    auto& sys_info = GlobalState::GetInstance()->SysInfo();
    sys_info.device_id =
        "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

    ForceCatalogPermissionRuleForTesting();

    SetDefaultStringPref(prefs::kDiagnosticId,
                         "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

    AdvanceClockTo(
        TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));
  }
};

TEST_F(BraveAdsConfirmationUserDataBuilderTest, BuildRewardConfirmation) {
  // Arrange
  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction, base::BindOnce([](const UserDataInfo& user_data) {
        // Assert
        EXPECT_EQ(
            base::test::ParseJsonDict(
                R"~({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"2020-11-18T12:00:00.000Z"})~"),
            user_data.dynamic);

        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data.fixed, &json));
        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"573c74fa-623a-4a46-adce-e688dfb7e8f5"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":".{44}","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConversionRewardConfirmation) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt);
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kConversion, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction, base::BindOnce([](const UserDataInfo& user_data) {
        // Assert
        EXPECT_EQ(
            base::test::ParseJsonDict(
                R"~({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"2020-11-18T12:00:00.000Z"})~"),
            user_data.dynamic);

        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data.fixed, &json));
        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"573c74fa-623a-4a46-adce-e688dfb7e8f5"}],"conversion":\[{"action":"view"}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":".{44}","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildVerifiableConversionRewardConfirmation) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kClicked,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kConversion, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction, base::BindOnce([](const UserDataInfo& user_data) {
        // Assert
        EXPECT_EQ(
            base::test::ParseJsonDict(
                R"~({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"2020-11-18T12:00:00.000Z"})~"),
            user_data.dynamic);

        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data.fixed, &json));
        const std::string pattern =
            R"~({"buildChannel":"release","catalog":\[{"id":"573c74fa-623a-4a46-adce-e688dfb7e8f5"}],"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":".{44}","segment":"untargeted","studies":\[],"versionNumber":"\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}"})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest, BuildNonRewardConfirmation) {
  // Arrange
  DisableBraveRewardsForTesting();

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(transaction,
                            base::BindOnce([](const UserDataInfo& user_data) {
                              // Assert
                              EXPECT_TRUE(user_data.dynamic.empty());
                              EXPECT_TRUE(user_data.fixed.empty());
                            }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConversionNonRewardConfirmation) {
  // Arrange
  DisableBraveRewardsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt);
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kConversion, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction, base::BindOnce([](const UserDataInfo& user_data) {
        // Assert
        EXPECT_TRUE(user_data.dynamic.empty());

        EXPECT_EQ(base::test::ParseJsonDict(
                      R"~({"conversion":[{"action":"view"}]})~"),
                  user_data.fixed);
      }));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildVerifiableConversionNonRewardConfirmation) {
  // Arrange
  DisableBraveRewardsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ false);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kClicked,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItemsForTesting(conversion, /*count*/ 1);
  SaveConversionQueueItemsForTesting(conversion_queue_items);

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kConversion, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction, base::BindOnce([](const UserDataInfo& user_data) {
        // Assert
        EXPECT_TRUE(user_data.dynamic.empty());

        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data.fixed, &json));
        const std::string pattern =
            R"~({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

}  // namespace brave_ads

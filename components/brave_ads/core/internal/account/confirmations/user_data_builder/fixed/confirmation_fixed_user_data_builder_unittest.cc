/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/fixed/confirmation_fixed_user_data_builder.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/build_user_data_callback.h"
#include "brave/components/brave_ads/core/internal/browser/browser_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsFixedUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(
        TimeFromString("November 18 2020 12:34:56.789", /*is_local=*/false));
  }
};

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildConfirmationUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  const base::Value::Dict expected_user_data =
      base::test::ParseJsonDict(base::ReplaceStringPlaceholders(
          R"(
              {
                "buildChannel": "release",
                "catalog": [
                  {
                    "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
                  }
                ],
                "countryCode": "US",
                "createdAtTimestamp": "2020-11-18T12:00:00.000Z",
                "platform": "windows",
                "rotating_hash": "I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",
                "segment": "untargeted",
                "studies": [],
                "topSegment": [],
                "versionNumber": "$1"
              })",
          {GetBrowserVersionNumber()}, nullptr));

  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_user_data))));
  BuildFixedUserData(transaction, callback.Get());
}

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(/*user_data=*/::testing::IsEmpty()));
  BuildFixedUserData(transaction, callback.Get());
}

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildConversionConfirmationUserDataForRewardsUser) {
  // Arrange
  test::BuildAndSaveConversionQueue(
      AdType::kNotificationAd, ConfirmationType::kViewed,
      /*is_verifiable=*/false, /*should_use_random_uuids=*/false, /*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  const base::Value::Dict expected_user_data =
      base::test::ParseJsonDict(base::ReplaceStringPlaceholders(
          R"(
              {
                "buildChannel": "release",
                "catalog": [
                  {
                    "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
                  }
                ],
                "conversion": [
                  {
                    "action": "view"
                  }
                ],
                "countryCode": "US",
                "createdAtTimestamp": "2020-11-18T12:00:00.000Z",
                "platform": "windows",
                "rotating_hash": "I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",
                "segment": "untargeted",
                "studies": [],
                "versionNumber": "$1"
              })",
          {GetBrowserVersionNumber()}, nullptr));

  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_user_data))));
  BuildFixedUserData(transaction, callback.Get());
}

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildConversionConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  test::BuildAndSaveConversionQueue(
      AdType::kNotificationAd, ConfirmationType::kClicked,
      /*is_verifiable=*/false, /*should_use_random_uuids=*/false, /*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "conversion": [
              {
                "action": "click"
              }
            ]
          })");

  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_user_data))));
  BuildFixedUserData(transaction, callback.Get());
}

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildVerifiableConversionConfirmationUserDataForRewardsUser) {
  // Arrange
  test::BuildAndSaveConversionQueue(
      AdType::kNotificationAd, ConfirmationType::kViewed,
      /*is_verifiable=*/true, /*should_use_random_uuids=*/false, /*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([](base::Value::Dict user_data) {
    std::string json;
    ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));
    const std::string pattern = base::ReplaceStringPlaceholders(
        R"({"buildChannel":"release","catalog":\[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"conversion":\[{"action":"view"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}],"countryCode":"US","createdAtTimestamp":"2020-11-18T12:00:00.000Z","platform":"windows","rotating_hash":"I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=","segment":"untargeted","studies":\[],"versionNumber":"$1"})",
        {GetBrowserVersionNumber()}, nullptr);
    EXPECT_TRUE(RE2::FullMatch(json, pattern));
  });

  BuildFixedUserData(transaction, callback.Get());
}

TEST_F(BraveAdsFixedUserDataBuilderTest,
       BuildVerifiableConversionConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  test::BuildAndSaveConversionQueue(
      AdType::kNotificationAd, ConfirmationType::kClicked,
      /*is_verifiable=*/true, /*should_use_random_uuids=*/false, /*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kConversion,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([](base::Value::Dict user_data) {
    std::string json;
    ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));
    const std::string pattern =
        R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})";
    EXPECT_TRUE(RE2::FullMatch(json, pattern));
  });

  BuildFixedUserData(transaction, callback.Get());
}

}  // namespace brave_ads

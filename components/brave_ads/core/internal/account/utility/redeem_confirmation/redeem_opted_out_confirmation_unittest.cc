/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_opted_out_confirmation.h"

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

using ::testing::_;
using ::testing::NiceMock;

class BatAdsRedeemOptedOutConfirmationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ads_client_mock_->SetBooleanPref(prefs::kEnabled, false);
  }

  NiceMock<RedeemConfirmationDelegateMock> redeem_confirmation_delegate_mock_;

  base::WeakPtrFactory<RedeemConfirmationDelegateMock>
      confirmation_delegate_weak_factory_{&redeem_confirmation_delegate_mock_};
};

TEST_F(BatAdsRedeemOptedOutConfirmationTest, Redeem) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::kHttpImATeapot, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(*confirmation));

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(_, _, _))
      .Times(0);

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BatAdsRedeemOptedOutConfirmationTest,
       DoNotRetryRedeemingForHttpBadRequestResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_BAD_REQUEST, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BatAdsRedeemOptedOutConfirmationTest,
       DoNotRetryRedeemingForHttpConflictResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_CONFLICT, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BatAdsRedeemOptedOutConfirmationTest,
       DoNotRetryReemingForHttpCreatedResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_CREATED, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(
      redeem_confirmation_delegate_mock_,
      OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BatAdsRedeemOptedOutConfirmationTest, RetryRedeeming) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a",
       {{net::HTTP_INTERNAL_SERVER_ERROR, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(*confirmation, /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedOutConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

}  // namespace brave_ads

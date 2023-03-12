/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

namespace {

constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kConversionId[] = "smartbrownfoxes42";
constexpr char kAdvertiserPublicKey[] =
    "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";

}  // namespace

class BatAdsConversionUserDataTest : public UnitTestBase {};

TEST_F(BatAdsConversionUserDataTest, GetForConversionConfirmationType) {
  // Arrange
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Act
  GetConversion(
      kCreativeInstanceId, ConfirmationType::kConversion,
      base::BindOnce([](base::Value::Dict user_data) {
        const absl::optional<security::VerifiableConversionEnvelopeInfo>
            verifiable_conversion_envelope =
                security::GetVerifiableConversionEnvelopeForUserData(user_data);
        ASSERT_TRUE(verifiable_conversion_envelope);
      }));

  // Assert
}

TEST_F(BatAdsConversionUserDataTest, DoNotGetForNonConversionConfirmationType) {
  // Arrange
  BuildAndSaveConversionQueueItem(kConversionId, kAdvertiserPublicKey);

  // Act
  GetConversion(
      kCreativeInstanceId, ConfirmationType::kViewed,
      base::BindOnce([](base::Value::Dict user_data) {
        const absl::optional<security::VerifiableConversionEnvelopeInfo>
            verifiable_conversion_envelope =
                security::GetVerifiableConversionEnvelopeForUserData(user_data);
        ASSERT_FALSE(verifiable_conversion_envelope);
      }));

  // Assert
}

}  // namespace ads::user_data

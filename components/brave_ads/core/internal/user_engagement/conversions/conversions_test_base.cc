/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_base.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads::test {

BraveAdsConversionsTestBase::BraveAdsConversionsTestBase() = default;

BraveAdsConversionsTestBase::~BraveAdsConversionsTestBase() = default;

void BraveAdsConversionsTestBase::SetUp() {
  TestBase::SetUp();

  conversions_ = std::make_unique<Conversions>();
  conversions_->AddObserver(&conversions_observer_mock_);
}

void BraveAdsConversionsTestBase::TearDown() {
  conversions_->RemoveObserver(&conversions_observer_mock_);

  TestBase::TearDown();
}

void BraveAdsConversionsTestBase::VerifyOnDidConvertAdExpectation(
    const AdInfo& ad,
    ConversionActionType action_type) {
  EXPECT_CALL(conversions_observer_mock_,
              OnDidConvertAd(/*conversion=*/::testing::FieldsAre(
                  ad.type, ad.creative_instance_id, ad.creative_set_id,
                  ad.campaign_id, ad.advertiser_id, ad.segment, action_type,
                  /*verifiable*/ std::nullopt)));
}

void BraveAdsConversionsTestBase::VerifyOnDidNotConvertAdExpectation() {
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd).Times(0);
}

void BraveAdsConversionsTestBase::VerifyOnDidConvertVerifiableAdExpectation(
    const AdInfo& ad,
    ConversionActionType action_type,
    const VerifiableConversionInfo& verifiable_conversion) {
  EXPECT_CALL(
      conversions_observer_mock_,
      OnDidConvertAd(/*conversion=*/::testing::FieldsAre(
          ad.type, ad.creative_instance_id, ad.creative_set_id, ad.campaign_id,
          ad.advertiser_id, ad.segment, action_type, verifiable_conversion)));
}

}  // namespace brave_ads::test

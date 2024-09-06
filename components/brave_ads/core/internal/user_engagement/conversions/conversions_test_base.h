/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_BASE_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer_mock.h"

namespace brave_ads {

class Conversions;
struct AdInfo;
struct VerifiableConversionInfo;

namespace test {

class BraveAdsConversionsTestBase : public TestBase {
 public:
  BraveAdsConversionsTestBase();

  BraveAdsConversionsTestBase(const BraveAdsConversionsTestBase&) = delete;
  BraveAdsConversionsTestBase& operator=(const BraveAdsConversionsTestBase&) =
      delete;

  BraveAdsConversionsTestBase(BraveAdsConversionsTestBase&&) noexcept = delete;
  BraveAdsConversionsTestBase& operator=(
      BraveAdsConversionsTestBase&&) noexcept = delete;

  ~BraveAdsConversionsTestBase() override;

  // TestBase:
  void SetUp() override;
  void TearDown() override;

 protected:
  void VerifyOnDidConvertAdExpectation(const AdInfo& ad,
                                       ConversionActionType action_type);

  void VerifyOnDidNotConvertAdExpectation();

  void VerifyOnDidConvertVerifiableAdExpectation(
      const AdInfo& ad,
      ConversionActionType action_type,
      const VerifiableConversionInfo& verifiable_conversion);

  std::unique_ptr<Conversions> conversions_;

  ConversionsObserverMock conversions_observer_mock_;
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_TEST_BASE_H_

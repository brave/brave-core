/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UNITTEST_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UNITTEST_BASE_H_

#include <memory>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer_mock.h"

namespace brave_ads {

class Conversions;
struct AdInfo;
struct VerifiableConversionInfo;

class BraveAdsConversionsUnitTestBase : public UnitTestBase {
 public:
  BraveAdsConversionsUnitTestBase();

  BraveAdsConversionsUnitTestBase(const BraveAdsConversionsUnitTestBase&) =
      delete;
  BraveAdsConversionsUnitTestBase& operator=(
      const BraveAdsConversionsUnitTestBase&) = delete;

  BraveAdsConversionsUnitTestBase(BraveAdsConversionsUnitTestBase&&) noexcept =
      delete;
  BraveAdsConversionsUnitTestBase& operator=(
      BraveAdsConversionsUnitTestBase&&) noexcept = delete;

  ~BraveAdsConversionsUnitTestBase() override;

  // UnitTestBase:
  void SetUp() override;
  void TearDown() override;

 protected:
  void RecordAdEventsAdvancingTheClockAfterEach(
      const AdInfo& ad,
      const std::vector<ConfirmationType>& confirmation_types);

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

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_UNITTEST_BASE_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/test/fake_ads_factory.h"

#include <utility>

#include "base/test/gmock_callback_support.h"
#include "brave/components/brave_ads/core/public/test/ads_mock.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads::test {

FakeAdsFactory::FakeAdsFactory() = default;

FakeAdsFactory::~FakeAdsFactory() = default;

std::unique_ptr<Ads> FakeAdsFactory::CreateAds(
    AdsClient& /*ads_client*/,
    const base::FilePath& /*database_path*/) {
  ++create_count_;

  auto ads = std::make_unique<testing::NiceMock<AdsMock>>();

  ON_CALL(*ads, Initialize)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<1>(
          !simulate_initialization_failure_));
  ON_CALL(*ads, Shutdown)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<0>(
          !simulate_shutdown_failure_));
  ON_CALL(*ads, GetInternals)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<0>(
          /*internals=*/std::nullopt));
  ON_CALL(*ads, GetDiagnostics)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<0>(
          /*diagnostics=*/std::nullopt));
  ON_CALL(*ads, EvaluateConditionMatcher)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<3>(
          /*current_value=*/"Unknown", /*matches=*/"N/A"));
  ON_CALL(*ads, GetStatementOfAccounts)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<0>(
          /*mojom_statement=*/nullptr));
  ON_CALL(*ads, ParseAndSaveNewTabPageAds)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, MaybeServeNewTabPageAd)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<0>(/*ad=*/std::nullopt));
  ON_CALL(*ads, TriggerNewTabPageAdEvent)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<4>(/*success=*/false));
  ON_CALL(*ads, MaybeGetNotificationAd)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*ad=*/std::nullopt));
  ON_CALL(*ads, TriggerNotificationAdEvent)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<2>(/*success=*/false));
  ON_CALL(*ads, MaybeGetSearchResultAd)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<1>(
          /*mojom_creative_ad=*/nullptr));
  ON_CALL(*ads, TriggerSearchResultAdEvent)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<2>(/*success=*/false));
  ON_CALL(*ads, PurgeOrphanedAdEventsForType)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, GetAdHistory)
      .WillByDefault(base::test::RunOnceCallbackRepeatedly<2>(
          /*mojom_ad_history=*/std::nullopt));
  ON_CALL(*ads, ToggleLikeAd)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, ToggleDislikeAd)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, ToggleLikeSegment)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, ToggleDislikeSegment)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, ToggleSaveAd)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));
  ON_CALL(*ads, ToggleMarkAdAsInappropriate)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<1>(/*success=*/false));

  ads_ = ads->GetWeakPtr();
  return std::move(ads);
}

}  // namespace brave_ads::test

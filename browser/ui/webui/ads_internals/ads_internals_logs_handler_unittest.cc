// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_logs_handler.h"

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/browser/test/fake_rewards_service.h"
#include "brave/components/brave_rewards/core/features.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "components/webui/flags/pref_service_flags_storage.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=AdsInternalsLogsHandlerTest*

namespace {

// `RewardsService::LoadDiagnosticLog` treats this as "no limit, return the
// full log".
constexpr int kFullLogNumLines = -1;

class AdsInternalsLogsHandlerTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(AdsInternalsLogsHandlerTest,
       GetLogWithNullRewardsServiceReturnsEmptyLog) {
  // Arrange
  AdsInternalsLogsHandler handler(/*rewards_service=*/nullptr,
                                  /*local_state=*/nullptr);

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  remote->GetLog(
      /*num_lines=*/5000,
      base::BindLambdaForTesting([&test_future](const std::string& log) {
        test_future.SetValue(log);
      }));

  // Assert
  EXPECT_EQ("", test_future.Get());
}

TEST_F(AdsInternalsLogsHandlerTest,
       ClearLogWithNullRewardsServiceRunsCallbackWithFalse) {
  // Arrange
  AdsInternalsLogsHandler handler(/*rewards_service=*/nullptr,
                                  /*local_state=*/nullptr);

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<bool> test_future;

  // Act
  remote->ClearLog(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(AdsInternalsLogsHandlerTest, GetLogDelegatesCallbackToRewardsService) {
  // Arrange
  brave_ads::test::FakeRewardsService rewards_service;
  AdsInternalsLogsHandler handler(&rewards_service, /*local_state=*/nullptr);

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  remote->GetLog(
      /*num_lines=*/5000,
      base::BindLambdaForTesting([&test_future](const std::string& log) {
        test_future.SetValue(log);
      }));

  // Assert
  EXPECT_EQ("fake diagnostic log", test_future.Get());
}

TEST_F(AdsInternalsLogsHandlerTest, ClearLogDelegatesCallbackToRewardsService) {
  // Arrange
  brave_ads::test::FakeRewardsService rewards_service;
  AdsInternalsLogsHandler handler(&rewards_service, /*local_state=*/nullptr);

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<bool> test_future;

  // Act
  remote->ClearLog(test_future.GetCallback());

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(AdsInternalsLogsHandlerTest,
       GetLogWithNullNumLinesRequestsFullLogFromRewardsService) {
  // Arrange
  brave_ads::test::FakeRewardsService rewards_service;
  AdsInternalsLogsHandler handler(&rewards_service, /*local_state=*/nullptr);

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  remote->GetLog(
      /*num_lines=*/std::nullopt,
      base::BindLambdaForTesting([&test_future](const std::string& log) {
        test_future.SetValue(log);
      }));

  // Assert
  EXPECT_TRUE(test_future.Wait());
  EXPECT_THAT(rewards_service.last_load_diagnostic_log_num_lines(),
              testing::Optional(kFullLogNumLines));
}

TEST_F(AdsInternalsLogsHandlerTest,
       ToggleVerboseLoggingAndRestartWithFeatureDisabledEnablesFlag) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      brave_rewards::features::kVerboseLoggingFeature);

  TestingPrefServiceSimple local_state;
  flags_ui::PrefServiceFlagsStorage::RegisterPrefs(local_state.registry());

  AdsInternalsLogsHandler handler(/*rewards_service=*/nullptr, &local_state);

  base::test::TestFuture<void> test_future;
  handler.SetAttemptRestartCallbackForTesting(
      test_future.GetRepeatingCallback());

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  // Act
  remote->ToggleVerboseLoggingAndRestart();

  // Assert
  EXPECT_TRUE(test_future.Wait());
  flags_ui::PrefServiceFlagsStorage flags_storage(&local_state);
  EXPECT_THAT(flags_storage.GetFlags(),
              testing::Contains("brave-rewards-verbose-logging@1"));
}

TEST_F(AdsInternalsLogsHandlerTest,
       ToggleVerboseLoggingAndRestartWithFeatureEnabledDisablesFlag) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_rewards::features::kVerboseLoggingFeature);

  TestingPrefServiceSimple local_state;
  flags_ui::PrefServiceFlagsStorage::RegisterPrefs(local_state.registry());

  AdsInternalsLogsHandler handler(/*rewards_service=*/nullptr, &local_state);

  base::test::TestFuture<void> test_future;
  handler.SetAttemptRestartCallbackForTesting(
      test_future.GetRepeatingCallback());

  mojo::Remote<bat_ads::mojom::AdsInternalsLogs> remote;
  handler.BindInterface(remote.BindNewPipeAndPassReceiver());

  // Act
  remote->ToggleVerboseLoggingAndRestart();

  // Assert
  EXPECT_TRUE(test_future.Wait());
  flags_ui::PrefServiceFlagsStorage flags_storage(&local_state);
  // Selecting the "Default" choice clears the override rather than storing
  // it, since the feature's built-in state (disabled) already matches.
  EXPECT_THAT(flags_storage.GetFlags(), testing::IsEmpty());
}

}  // namespace

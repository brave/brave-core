// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_callback.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsInternalsHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    profile_prefs_.registry()->RegisterBooleanPref(
        brave_rewards::prefs::kEnabled, false);
    profile_prefs_.registry()->RegisterStringPref(prefs::kDiagnosticId, "");
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple profile_prefs_;
  AdsServiceMock ads_service_mock_;
};

TEST_F(BraveAdsInternalsHandlerTest,
       GetAdsInternalsWithNullAdsServiceReturnsEmptyJson) {
  // Arrange
  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetAdsInternals(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ("{}", test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetAdsInternalsDelegatesCallbackToAdsService) {
  // Arrange
  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, GetInternals)
      .WillOnce([](GetInternalsCallback callback) {
        std::move(callback).Run(base::DictValue().Set("foo", "bar"));
      });

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetAdsInternals(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(R"JSON({"foo":"bar"})JSON", test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetAdsInternalsWithNulloptInternalsReturnsEmptyJson) {
  // Arrange
  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, GetInternals)
      .WillOnce([](GetInternalsCallback callback) {
        std::move(callback).Run(/*dict=*/std::nullopt);
      });

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetAdsInternals(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ("{}", test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       ClearAdsDataWithNullAdsServiceRunsCallbackWithFalse) {
  // Arrange
  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<bool> test_future;

  // Act
  ads_internals_remote->ClearAdsData(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       ClearAdsDataDelegatesCallbackToAdsService) {
  // Arrange
  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, ClearData)
      .WillOnce([](ResultCallback callback) {
        std::move(callback).Run(/*success=*/true);
      });

  base::test::TestFuture<bool> test_future;

  // Act
  ads_internals_remote->ClearAdsData(test_future.GetCallback());

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       ClearAdsDataRunsCallbackWithFalseOnAdsServiceFailure) {
  // Arrange
  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, ClearData)
      .WillOnce([](ResultCallback callback) {
        std::move(callback).Run(/*success=*/false);
      });

  base::test::TestFuture<bool> test_future;

  // Act
  ads_internals_remote->ClearAdsData(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetDiagnosticsWithNullAdsServiceReturnsDiagnosticIdOnly) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetDiagnostics(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(R"JSON({"diagnosticId":"diagnostic-id"})JSON", test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetDiagnosticsDelegatesCallbackToAdsService) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, GetDiagnostics)
      .WillOnce([](GetDiagnosticsCallback callback) {
        base::ListValue entries;
        entries.Append(
            base::DictValue().Set("name", "foo").Set("value", "bar"));
        std::move(callback).Run(std::move(entries));
      });

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetDiagnostics(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(R"JSON({"diagnosticId":"diagnostic-id",)JSON"
            R"JSON("entries":[{"name":"foo","value":"bar"}]})JSON",
            test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetDiagnosticsWithNulloptEntriesReturnsDiagnosticIdOnly) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(&ads_service_mock_, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, GetDiagnostics)
      .WillOnce([](GetDiagnosticsCallback callback) {
        std::move(callback).Run(/*diagnostics=*/std::nullopt);
      });

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetDiagnostics(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(R"JSON({"diagnosticId":"diagnostic-id"})JSON", test_future.Get());
}

TEST_F(BraveAdsInternalsHandlerTest, SetDiagnosticIdUpdatesPrefWithValidUuid) {
  // Arrange
  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  const std::string diagnostic_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  // Act
  ads_internals_remote->SetDiagnosticId(diagnostic_id);
  ads_internals_remote.FlushForTesting();

  // Assert
  EXPECT_EQ(diagnostic_id, profile_prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsInternalsHandlerTest,
       SetDiagnosticIdIgnoresNonUuidFormattedValue) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  // Act
  ads_internals_remote->SetDiagnosticId("not-a-uuid");
  ads_internals_remote.FlushForTesting();

  // Assert
  EXPECT_EQ("diagnostic-id", profile_prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsInternalsHandlerTest, SetDiagnosticIdIgnoresEmptyValue) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(/*ads_service=*/nullptr, profile_prefs_);

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  // Act
  ads_internals_remote->SetDiagnosticId("");
  ads_internals_remote.FlushForTesting();

  // Assert
  EXPECT_EQ("diagnostic-id", profile_prefs_.GetString(prefs::kDiagnosticId));
}

}  // namespace brave_ads

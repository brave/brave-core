// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_callback.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

// The device locale is not pinned in this test suite, so the fallback country
// code must be computed the same way `AdsInternalsHandler` computes it rather
// than hardcoded, otherwise the expectation depends on the test machine's
// locale.
std::string ExpectedFallbackVariationsCountryCode() {
  return base::StrCat(
      {base::ToUpperASCII(CurrentCountryCode()), " (Fallback)"});
}

}  // namespace

class BraveAdsInternalsHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    profile_prefs_.registry()->RegisterBooleanPref(
        brave_rewards::prefs::kEnabled, false);
    profile_prefs_.registry()->RegisterStringPref(
        brave_rewards::prefs::kExternalWalletType, "");
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
  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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

  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetDiagnostics(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(absl::StrFormat(
                R"JSON(
                {
                  "diagnosticId": "diagnostic-id",
                  "isInitialized": false,
                  "variationsCountryCode": "%s"
                })JSON",
                ExpectedFallbackVariationsCountryCode())),
            base::test::ParseJsonDict(test_future.Get()));
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetDiagnosticsDelegatesCallbackToAdsService) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  EXPECT_CALL(ads_service_mock_, GetDiagnostics)
      .WillOnce([](GetDiagnosticsCallback callback) {
        base::ListValue entries;
        entries.Append(
            base::DictValue().Set("name", "foo").Set("value", "bar"));
        std::move(callback).Run(
            base::DictValue().Set("entries", std::move(entries)));
      });

  base::test::TestFuture<std::string> test_future;

  // Act
  ads_internals_remote->GetDiagnostics(base::BindLambdaForTesting(
      [&test_future](const std::string& json) { test_future.SetValue(json); }));

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(absl::StrFormat(
                R"JSON(
                {
                  "diagnosticId": "diagnostic-id",
                  "entries": [{"name": "foo", "value": "bar"}],
                  "isInitialized": false,
                  "variationsCountryCode": "%s"
                })JSON",
                ExpectedFallbackVariationsCountryCode())),
            base::test::ParseJsonDict(test_future.Get()));
}

TEST_F(BraveAdsInternalsHandlerTest,
       GetDiagnosticsWithNulloptEntriesReturnsDiagnosticIdOnly) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(
      &ads_service_mock_, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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
  EXPECT_EQ(base::test::ParseJsonDict(absl::StrFormat(
                R"JSON(
                {
                  "diagnosticId": "diagnostic-id",
                  "isInitialized": false,
                  "variationsCountryCode": "%s"
                })JSON",
                ExpectedFallbackVariationsCountryCode())),
            base::test::ParseJsonDict(test_future.Get()));
}

TEST_F(BraveAdsInternalsHandlerTest, SetDiagnosticIdUpdatesPrefWithValidUuid) {
  // Arrange
  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

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

  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  // Act
  ads_internals_remote->SetDiagnosticId("not-a-uuid");
  ads_internals_remote.FlushForTesting();

  // Assert
  EXPECT_EQ("diagnostic-id", profile_prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsInternalsHandlerTest, SetDiagnosticIdClearsPrefWithEmptyValue) {
  // Arrange
  profile_prefs_.SetString(prefs::kDiagnosticId, "diagnostic-id");

  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  // Act
  ads_internals_remote->SetDiagnosticId("");
  ads_internals_remote.FlushForTesting();

  // Assert
  EXPECT_EQ("", profile_prefs_.GetString(prefs::kDiagnosticId));
}

namespace {

class FakeAdsInternalsPage final : public bat_ads::mojom::AdsInternalsPage {
 public:
  mojo::PendingRemote<bat_ads::mojom::AdsInternalsPage> BindNewPipe() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void FlushForTesting() { receiver_.FlushForTesting(); }

  std::optional<bool> last_rewards_enabled;
  std::optional<bool> last_wallet_connected;
  bool did_initialize_ads_service = false;

 private:
  // bat_ads::mojom::AdsInternalsPage:
  void UpdateBraveRewardsEnabled(bool enabled) override {
    last_rewards_enabled = enabled;
  }

  void UpdateBraveRewardsWalletConnected(bool connected) override {
    last_wallet_connected = connected;
  }

  void UpdateDidInitializeAdsService() override {
    did_initialize_ads_service = true;
  }

  mojo::Receiver<bat_ads::mojom::AdsInternalsPage> receiver_{this};
};

}  // namespace

TEST_F(BraveAdsInternalsHandlerTest,
       CreateAdsInternalsPageHandlerPushesInitialState) {
  // Arrange
  profile_prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");

  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  FakeAdsInternalsPage page;

  // Act
  ads_internals_remote->CreateAdsInternalsPageHandler(page.BindNewPipe());
  page.FlushForTesting();

  // Assert
  EXPECT_EQ(true, page.last_rewards_enabled);
  EXPECT_EQ(true, page.last_wallet_connected);
}

TEST_F(BraveAdsInternalsHandlerTest,
       WalletConnectedPrefChangePushesUpdatedState) {
  // Arrange
  AdsInternalsHandler handler(
      /*ads_service=*/nullptr, profile_prefs_,
      /*variations_service=*/nullptr,
      /*get_ntp_sponsored_images_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_country_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      /*get_language_resource_component_id_callback=*/
      AdsInternalsHandler::GetComponentIdCallback(),
      AdsInternalsHandler::GetIsSponsoredImagesLoadedCallback(),
      AdsInternalsHandler::GetComponentIdCallback());

  mojo::Remote<bat_ads::mojom::AdsInternals> ads_internals_remote;
  handler.BindInterface(ads_internals_remote.BindNewPipeAndPassReceiver());

  FakeAdsInternalsPage page;
  ads_internals_remote->CreateAdsInternalsPageHandler(page.BindNewPipe());
  page.FlushForTesting();
  ASSERT_EQ(false, page.last_wallet_connected);

  // Act
  profile_prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");
  page.FlushForTesting();

  // Assert
  EXPECT_EQ(true, page.last_wallet_connected);
}

}  // namespace brave_ads

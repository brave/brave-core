// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/cr_components/customize_color_scheme_mode/brave_customize_color_scheme_mode_handler.h"

#include <memory>

#include "base/test/test_future.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class MockClient : public customize_color_scheme_mode::mojom::
                       CustomizeColorSchemeModeClient {
 public:
  MockClient() = default;
  ~MockClient() override = default;

  mojo::PendingRemote<
      customize_color_scheme_mode::mojom::CustomizeColorSchemeModeClient>
  BindAndGetRemote() {
    CHECK(!receiver_.is_bound());
    return receiver_.BindNewPipeAndPassRemote();
  }

  MOCK_METHOD(void,
              SetColorSchemeMode,
              (customize_color_scheme_mode::mojom::ColorSchemeMode mode),
              (override));

 private:
  mojo::Receiver<
      customize_color_scheme_mode::mojom::CustomizeColorSchemeModeClient>
      receiver_{this};
};

class BraveCustomizeColorSchemeModeHandlerUnitTest : public testing::Test {
 protected:
  // testing::Test:
  void SetUp() override {
    handler_ = std::make_unique<BraveCustomizeColorSchemeModeHandler>(
        mock_client_.BindAndGetRemote(),
        mojo::PendingReceiver<customize_color_scheme_mode::mojom::
                                  CustomizeColorSchemeModeHandler>(),
        &testing_profile_);
    dark_mode::SetUseSystemDarkModeEnabledForTest(true);
  }

  void TearDown() override {
    handler_.reset();
    dark_mode::SetUseSystemDarkModeEnabledForTest(false);
  }

  sync_preferences::TestingPrefServiceSyncable* GetTestingPrefService() {
    return testing_profile_.GetTestingPrefService();
  }

  content::BrowserTaskEnvironment task_environment_;
  ScopedTestingLocalState local_state_{TestingBrowserProcess::GetGlobal()};
  TestingProfile testing_profile_;
  testing::NiceMock<MockClient> mock_client_;

  std::unique_ptr<BraveCustomizeColorSchemeModeHandler> handler_;
};

TEST_F(BraveCustomizeColorSchemeModeHandlerUnitTest,
       ClientSetColorSchemeModeShouldBeCalledWhenBraveDarkModeTypeChanges) {
  base::test::TestFuture<customize_color_scheme_mode::mojom::ColorSchemeMode>
      future;
  EXPECT_CALL(mock_client_, SetColorSchemeMode(testing::_))
      .WillOnce([&](auto mode) { future.SetValue(mode); });
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  EXPECT_EQ(future.Take(),
            customize_color_scheme_mode::mojom::ColorSchemeMode::kDark);
  testing::Mock::VerifyAndClearExpectations(&mock_client_);

  EXPECT_CALL(mock_client_, SetColorSchemeMode(testing::_))
      .WillOnce([&](auto mode) { future.SetValue(mode); });
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_EQ(future.Take(),
            customize_color_scheme_mode::mojom::ColorSchemeMode::kLight);
  testing::Mock::VerifyAndClearExpectations(&mock_client_);

  EXPECT_CALL(mock_client_, SetColorSchemeMode(testing::_))
      .WillOnce([&](auto mode) { future.SetValue(mode); });
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);
  EXPECT_EQ(future.Take(),
            customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem);
  testing::Mock::VerifyAndClearExpectations(&mock_client_);
}

TEST_F(
    BraveCustomizeColorSchemeModeHandlerUnitTest,
    ClientSetColorSchemeModeShouldGetValueDarkModeUtilInsteadOfThemeService) {
  auto* theme_service = ThemeServiceFactory::GetForProfile(&testing_profile_);
  ASSERT_TRUE(theme_service);
  ASSERT_EQ(dark_mode::GetBraveDarkModeType(),
            dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);

  // Color mode callback should be called with the value from dark_mode util,
  // which uses local state instead of ThemeService.
  base::test::TestFuture<customize_color_scheme_mode::mojom::ColorSchemeMode>
      future;
  EXPECT_CALL(mock_client_, SetColorSchemeMode(testing::_))
      .WillOnce([&](auto mode) { future.SetValue(mode); });
  theme_service->SetBrowserColorScheme(ThemeService::BrowserColorScheme::kDark);
  EXPECT_EQ(future.Get(),
            customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem);
}

TEST_F(
    BraveCustomizeColorSchemeModeHandlerUnitTest,
    InitializeColorSchemeModeShouldGetValueDarkModeUtilInsteadOfThemeService) {
  // Color mode callback should be called with the value from dark_mode util,
  // which uses local state instead of ThemeService.
  base::test::TestFuture<customize_color_scheme_mode::mojom::ColorSchemeMode>
      future;
  EXPECT_CALL(mock_client_, SetColorSchemeMode(testing::_))
      .WillOnce([&](auto mode) { future.SetValue(mode); });
  handler_->InitializeColorSchemeMode();
  EXPECT_EQ(future.Get(),
            customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem);
}

TEST_F(BraveCustomizeColorSchemeModeHandlerUnitTest,
       SetColorSchemeShouldSetColorSchemeUsingDarkModeUtil) {
  auto* theme_service = ThemeServiceFactory::GetForProfile(&testing_profile_);
  ASSERT_TRUE(theme_service);
  ASSERT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT,
            dark_mode::GetBraveDarkModeType());
  ASSERT_EQ(customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem,
            static_cast<customize_color_scheme_mode::mojom::ColorSchemeMode>(
                theme_service->GetBrowserColorScheme()));

  handler_->SetColorSchemeMode(
      customize_color_scheme_mode::mojom::ColorSchemeMode::kDark);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK,
            dark_mode::GetBraveDarkModeType())
      << "Calling SetColorSchemeMode should set the Brave dark mode type "
         "to dark";
  EXPECT_EQ(customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem,
            static_cast<customize_color_scheme_mode::mojom::ColorSchemeMode>(
                theme_service->GetBrowserColorScheme()))
      << "ThemeService should not be changed by SetColorSchemeMode";

  handler_->SetColorSchemeMode(
      customize_color_scheme_mode::mojom::ColorSchemeMode::kLight);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT,
            dark_mode::GetBraveDarkModeType())
      << "Calling SetColorSchemeMode should set the Brave dark mode type "
         "to light";
  EXPECT_EQ(customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem,
            static_cast<customize_color_scheme_mode::mojom::ColorSchemeMode>(
                theme_service->GetBrowserColorScheme()))
      << "ThemeService should not be changed by SetColorSchemeMode";

  handler_->SetColorSchemeMode(
      customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT,
            dark_mode::GetBraveDarkModeType())
      << "Calling SetColorSchemeMode should set the Brave dark mode type "
         "to default";
  EXPECT_EQ(customize_color_scheme_mode::mojom::ColorSchemeMode::kSystem,
            static_cast<customize_color_scheme_mode::mojom::ColorSchemeMode>(
                theme_service->GetBrowserColorScheme()))
      << "ThemeService should not be changed by SetColorSchemeMode";
}

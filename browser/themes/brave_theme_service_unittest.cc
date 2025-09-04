/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/themes/theme_service_observer.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/features.h"
#include "brave/browser/ui/darker_theme/pref_names.h"
#endif  // defined(TOOLKIT_VIEWS)

TEST(BraveThemeServiceTest, GetBraveThemeListTest) {
  dark_mode::SetUseSystemDarkModeEnabledForTest(true);
  base::Value::List list = dark_mode::GetBraveDarkModeTypeList();
  EXPECT_EQ(3UL, list.size());

  dark_mode::SetUseSystemDarkModeEnabledForTest(false);
  list = dark_mode::GetBraveDarkModeTypeList();
  EXPECT_EQ(2UL, list.size());
}

#if defined(TOOLKIT_VIEWS)
class BraveThemeServiceDarkerThemeTest : public testing::Test {
 public:
  BraveThemeServiceDarkerThemeTest()
      : scoped_feature_list_(darker_theme::features::kBraveDarkerTheme) {}
  ~BraveThemeServiceDarkerThemeTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();

    theme_service_ = ThemeServiceFactory::GetForProfile(profile_.get());
    ASSERT_TRUE(theme_service_);

    pref_service_ = profile_->GetPrefs();
    ASSERT_TRUE(pref_service_);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;

  raw_ptr<ThemeService> theme_service_ = nullptr;
  raw_ptr<PrefService> pref_service_ = nullptr;
};

class MockThemeServiceObserver : public ThemeServiceObserver {
 public:
  // Called when the user has changed the browser theme.
  MOCK_METHOD(void, OnThemeChanged, (), (override));
};

TEST_F(BraveThemeServiceDarkerThemeTest,
       DarkerThemePrefChangeTriggersThemeChange) {
  bool value = pref_service_->GetBoolean(darker_theme::prefs::kBraveDarkerMode);
  testing::NiceMock<MockThemeServiceObserver> observer;
  theme_service_->AddObserver(&observer);

  // When changing the pref, we should get a theme change notification.
  EXPECT_CALL(observer, OnThemeChanged()).Times(1);
  pref_service_->SetBoolean(darker_theme::prefs::kBraveDarkerMode, !value);
  testing::Mock::VerifyAndClearExpectations(&observer);

  // When resetting the pref, we should get a theme change notification.
  EXPECT_CALL(observer, OnThemeChanged()).Times(1);
  pref_service_->SetBoolean(darker_theme::prefs::kBraveDarkerMode, value);
  testing::Mock::VerifyAndClearExpectations(&observer);

  theme_service_->RemoveObserver(&observer);
}
#endif  // defined(TOOLKIT_VIEWS)

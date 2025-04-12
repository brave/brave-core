/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"

#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=NTPWidgetUtilsRegionUtilTest.*

class NTPWidgetUtilsRegionUtilTest : public testing::Test {
 protected:
  void SetUp() override { profile_ = std::make_unique<TestingProfile>(); }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(NTPWidgetUtilsRegionUtilTest, TestRegionAllowedAllowList) {
  std::vector<std::string> supported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "US";
  auto id = country_codes::CountryId(code);

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(country_codes::kCountryIDAtInstall,
                                  id.Serialize());

  bool is_supported = ::ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), supported_regions, true);
  ASSERT_TRUE(is_supported);
}

TEST_F(NTPWidgetUtilsRegionUtilTest, TestRegionUnAllowedAllowList) {
  std::vector<std::string> supported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "JP";
  auto id = country_codes::CountryId(code);

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(country_codes::kCountryIDAtInstall,
                                  id.Serialize());

  bool is_supported = ::ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), supported_regions, true);
  ASSERT_FALSE(is_supported);
}

TEST_F(NTPWidgetUtilsRegionUtilTest, TestRegionAllowedDenyList) {
  std::vector<std::string> unsupported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "JP";
  auto id = country_codes::CountryId(code);

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(country_codes::kCountryIDAtInstall,
                                  id.Serialize());

  bool is_supported = ::ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), unsupported_regions, false);
  ASSERT_TRUE(is_supported);
}

TEST_F(NTPWidgetUtilsRegionUtilTest, TestRegionUnAllowedDenyList) {
  std::vector<std::string> unsupported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "US";
  auto id = country_codes::CountryId(code);

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(country_codes::kCountryIDAtInstall,
                                  id.Serialize());

  bool is_supported = ::ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), unsupported_regions, false);
  ASSERT_FALSE(is_supported);
}

TEST_F(NTPWidgetUtilsRegionUtilTest, TestFindLocaleInListOne) {
  {
    // Base test with default english locale
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_US"};
    EXPECT_EQ(::ntp_widget_utils::FindLocale({"en", "fr", "ja"}, "en"), "en");
  }

  {
    // Test that FindLocale returns set locale if it's in the provided list
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"ja_JP"};
    EXPECT_EQ(::ntp_widget_utils::FindLocale({"en", "fr", "ja"}, "en"), "ja");
  }

  {
    // Test that FindLocale returns the provided default locale if it's not in
    // the provided list
    brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"ar_DZ"};
    EXPECT_EQ(::ntp_widget_utils::FindLocale({"en", "fr", "ja"}, "en"), "en");
  }
}

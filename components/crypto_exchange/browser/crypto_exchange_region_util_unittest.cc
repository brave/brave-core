/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/crypto_exchange/browser/crypto_exchange_region_util.h"

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "components/country_codes/country_codes.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=CryptoExchangeRegionUtilTest.*

class CryptoExchangeRegionUtilTest : public testing::Test {
 protected:
  void SetUp() override {
    profile_.reset(new TestingProfile());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(CryptoExchangeRegionUtilTest, TestRegionAllowedAllowList) {
  std::vector<std::string> supported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "US";
  const int32_t id = country_codes::CountryCharsToCountryID(
    code.at(0), code.at(1));

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, id);

  bool is_supported = ::crypto_exchange::IsRegionSupported(
      profile->GetPrefs(), supported_regions, true);
  ASSERT_TRUE(is_supported);
}

TEST_F(CryptoExchangeRegionUtilTest, TestRegionUnAllowedAllowList) {
  std::vector<std::string> supported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "JP";
  const int32_t id = country_codes::CountryCharsToCountryID(
    code.at(0), code.at(1));

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, id);

  bool is_supported = ::crypto_exchange::IsRegionSupported(
      profile->GetPrefs(), supported_regions, true);
  ASSERT_FALSE(is_supported);
}

TEST_F(CryptoExchangeRegionUtilTest, TestRegionAllowedDenyList) {
  std::vector<std::string> unsupported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "JP";
  const int32_t id = country_codes::CountryCharsToCountryID(
    code.at(0), code.at(1));

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, id);

  bool is_supported = ::crypto_exchange::IsRegionSupported(
      profile->GetPrefs(), unsupported_regions, false);
  ASSERT_TRUE(is_supported);
}

TEST_F(CryptoExchangeRegionUtilTest, TestRegionUnAllowedDenyList) {
  std::vector<std::string> unsupported_regions = {
    "US",
    "CA",
    "GB",
    "DE"
  };
  const std::string code = "US";
  const int32_t id = country_codes::CountryCharsToCountryID(
    code.at(0), code.at(1));

  auto* profile = profile_.get();
  profile->GetPrefs()->SetInteger(
      country_codes::kCountryIDAtInstall, id);

  bool is_supported = ::crypto_exchange::IsRegionSupported(
      profile->GetPrefs(), unsupported_regions, false);
  ASSERT_FALSE(is_supported);
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "base/version_info/version_info.h"
#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider_util.h"
#include "brave/components/brave_ads/core/public/common/locale/scoped_locale_for_testing.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class VirtualPrefProviderDelegateMock : public VirtualPrefProvider::Delegate {
 public:
  MOCK_METHOD(std::string_view, GetChannel, (), (const));

  MOCK_METHOD(std::string, GetDefaultSearchEngineName, (), (const));
};

class BraveAdsVirtualPrefProviderTest : public ::testing::Test {
 public:
  BraveAdsVirtualPrefProviderTest() {
    prefs_.registry()->RegisterBooleanPref(
        ntp_background_images::prefs::kNewTabPageSponsoredImagesSurveyPanelist,
        true);
    local_state_.registry()->RegisterDictionaryPref(skus::prefs::kSkusState);

    auto delegate = std::make_unique<VirtualPrefProviderDelegateMock>();
    delegate_ = delegate.get();
    provider_ = std::make_unique<VirtualPrefProvider>(&prefs_, &local_state_,
                                                      std::move(delegate));
  }

  ~BraveAdsVirtualPrefProviderTest() override {
    // Reset delegate to avoid dangling pointer.
    delegate_ = nullptr;
  }

  VirtualPrefProviderDelegateMock& delegate() const { return *delegate_; }

  VirtualPrefProvider& provider() const { return *provider_; }

 protected:
  TestingPrefServiceSimple prefs_;
  TestingPrefServiceSimple local_state_;

  std::unique_ptr<VirtualPrefProvider> provider_;

  raw_ptr<VirtualPrefProviderDelegateMock> delegate_ = nullptr;
};

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserBuildChannel) {
  // Arrange
  EXPECT_CALL(delegate(), GetChannel).WillOnce(testing::Return("release"));

  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* build_channel =
      virtual_prefs.FindStringByDottedPath("[virtual]:browser.build_channel");
  ASSERT_TRUE(build_channel);

  // Assert
  EXPECT_EQ(*build_channel, "release");
}

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* version =
      virtual_prefs.FindStringByDottedPath("[virtual]:browser.version");
  ASSERT_TRUE(version);

  // Assert
  EXPECT_EQ(*version, version_info::GetVersionNumber());
}

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserMajorVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> major_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.major_version");

  // Assert
  EXPECT_EQ(major_version, GetMajorVersion());
}

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserMinorVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> minor_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.minor_version");

  // Assert
  EXPECT_EQ(minor_version, GetMinorVersion());
}

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserBuildVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> build_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.build_version");

  // Assert
  EXPECT_EQ(build_version, GetBuildVersion());
}

TEST_F(BraveAdsVirtualPrefProviderTest, BrowserPatchVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> patch_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.patch_version");

  // Assert
  EXPECT_EQ(patch_version, GetPatchVersion());
}

TEST_F(BraveAdsVirtualPrefProviderTest, OperatingSystemLocaleLanguage) {
  // Arrange
  const test::ScopedCurrentLanguageCode scoped_current_language_code{"en"};

  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* language = virtual_prefs.FindStringByDottedPath(
      "[virtual]:operating_system.locale.language");
  ASSERT_TRUE(language);

  // Assert
  EXPECT_EQ(*language, "en");
}

TEST_F(BraveAdsVirtualPrefProviderTest, OperatingSystemLocaleRegion) {
  // Arrange
  const test::ScopedCurrentCountryCode scoped_current_country_code{"KY"};

  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* region = virtual_prefs.FindStringByDottedPath(
      "[virtual]:operating_system.locale.region");
  ASSERT_TRUE(region);

  // Assert
  EXPECT_EQ(*region, "KY");
}

TEST_F(BraveAdsVirtualPrefProviderTest, OperatingSystemName) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* name =
      virtual_prefs.FindStringByDottedPath("[virtual]:operating_system.name");
  ASSERT_TRUE(name);

  // Assert
  EXPECT_EQ(*name, version_info::GetOSType());
}

TEST_F(BraveAdsVirtualPrefProviderTest, IsSurveyPanelist) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<bool> is_survey_panelist =
      virtual_prefs.FindBoolByDottedPath("[virtual]:is_survey_panelist");

  // Assert
  EXPECT_TRUE(is_survey_panelist);
}

TEST_F(BraveAdsVirtualPrefProviderTest, SearchEngineDefaultName) {
  // Arrange
  EXPECT_CALL(delegate(), GetDefaultSearchEngineName)
      .WillOnce(testing::Return("Brave"));

  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* default_search_engine_name =
      virtual_prefs.FindStringByDottedPath(
          "[virtual]:search_engine.default_name");
  ASSERT_TRUE(default_search_engine_name);

  // Assert
  EXPECT_EQ(*default_search_engine_name, "Brave");
}

TEST_F(BraveAdsVirtualPrefProviderTest, PrefsWithSkus) {
  // Arrange
  local_state_.SetDict(skus::prefs::kSkusState,
                       base::Value::Dict().Set("skus:development", R"JSON({
      "orders": {
        "f24787ab-7bc3-46b9-bc05-65befb360cb8": {
          "created_at": "2023-10-24T16:00:57.902289",
          "expires_at": "2023-12-24T17:03:59.030987",
          "last_paid_at": "2023-11-24T17:03:59.030987",
          "location": "leo.brave.com",
          "status": "paid"
        }
      }
    })JSON"));

  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const base::Value::Dict* skus = virtual_prefs.FindDict("[virtual]:skus");
  ASSERT_TRUE(skus);

  // Assert
  const base::Value::Dict expected_skus = base::test::ParseJsonDict(
      R"JSON(
      {
        "development": {
          "leo.brave.com": {
              "created_at": "2023-10-24T16:00:57.902289",
              "expires_at": "2023-12-24T17:03:59.030987",
              "last_paid_at": "2023-11-24T17:03:59.030987",
              "status": "paid"
          }
        }
      })JSON");
  EXPECT_EQ(expected_skus, *skus);
}

}  // namespace brave_ads

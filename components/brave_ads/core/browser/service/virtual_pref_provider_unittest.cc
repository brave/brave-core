/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "base/version_info/version_info.h"
#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider_util.h"
#include "brave/components/brave_ads/core/public/common/locale/scoped_locale_for_testing.h"
#include "brave/components/skus/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class VirtualPrefProviderDelegateMock : public VirtualPrefProvider::Delegate {
 public:
  MOCK_METHOD(std::string, GetChannel, (), (const));

  MOCK_METHOD(std::string, GetDefaultSearchEngineName, (), (const));
};

class VirtualPrefProviderTest : public ::testing::Test {
 public:
  VirtualPrefProviderTest() {
    auto delegate = std::make_unique<VirtualPrefProviderDelegateMock>();
    delegate_ = delegate.get();
    provider_ = std::make_unique<VirtualPrefProvider>(&pref_service_,
                                                      std::move(delegate));

    pref_service_.registry()->RegisterDictionaryPref(skus::prefs::kSkusState);
  }

  ~VirtualPrefProviderTest() override {
    // Reset delegate to avoid dangling pointer.
    delegate_ = nullptr;
  }

  TestingPrefServiceSimple& pref_service() { return pref_service_; }

  VirtualPrefProviderDelegateMock& delegate() const { return *delegate_; }

  VirtualPrefProvider& provider() const { return *provider_; }

 private:
  TestingPrefServiceSimple pref_service_;
  raw_ptr<VirtualPrefProviderDelegateMock> delegate_ = nullptr;
  std::unique_ptr<VirtualPrefProvider> provider_;
};

TEST_F(VirtualPrefProviderTest, BrowserBuildChannel) {
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

TEST_F(VirtualPrefProviderTest, BrowserVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* version =
      virtual_prefs.FindStringByDottedPath("[virtual]:browser.version");
  ASSERT_TRUE(version);

  // Assert
  EXPECT_EQ(*version, version_info::GetVersionNumber());
}

TEST_F(VirtualPrefProviderTest, BrowserMajorVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> major_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.major_version");

  // Assert
  EXPECT_EQ(major_version, GetMajorVersion());
}

TEST_F(VirtualPrefProviderTest, BrowserMinorVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> minor_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.minor_version");

  // Assert
  EXPECT_EQ(minor_version, GetMinorVersion());
}

TEST_F(VirtualPrefProviderTest, BrowserBuildVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> build_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.build_version");

  // Assert
  EXPECT_EQ(build_version, GetBuildVersion());
}

TEST_F(VirtualPrefProviderTest, BrowserPatchVersion) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<int> patch_version =
      virtual_prefs.FindIntByDottedPath("[virtual]:browser.patch_version");

  // Assert
  EXPECT_EQ(patch_version, GetPatchVersion());
}

TEST_F(VirtualPrefProviderTest, OperatingSystemLocaleLanguage) {
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

TEST_F(VirtualPrefProviderTest, OperatingSystemLocaleRegion) {
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

TEST_F(VirtualPrefProviderTest, OperatingSystemIsMobilePlatform) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  std::optional<bool> is_mobile_platform = virtual_prefs.FindBoolByDottedPath(
      "[virtual]:operating_system.is_mobile_platform");

  // Assert
  EXPECT_EQ(is_mobile_platform, IsMobilePlatform());
}

TEST_F(VirtualPrefProviderTest, OperatingSystemName) {
  // Act
  const base::Value::Dict virtual_prefs = provider().GetPrefs();
  const std::string* name =
      virtual_prefs.FindStringByDottedPath("[virtual]:operating_system.name");
  ASSERT_TRUE(name);

  // Assert
  EXPECT_EQ(*name, version_info::GetOSType());
}

TEST_F(VirtualPrefProviderTest, SearchEngineDefaultName) {
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

TEST_F(VirtualPrefProviderTest, PrefsWithSkus) {
  // Arrange
  pref_service().SetDict(skus::prefs::kSkusState,
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

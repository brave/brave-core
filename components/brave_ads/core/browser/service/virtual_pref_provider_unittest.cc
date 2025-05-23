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
#include "base/version.h"
#include "base/version_info/version_info.h"
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
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* build_channel =
      prefs.FindStringByDottedPath("[virtual]:browser.build_channel");

  // Assert
  ASSERT_TRUE(build_channel);
  EXPECT_EQ(*build_channel, "release");
}

TEST_F(VirtualPrefProviderTest, BrowserVersion) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* version =
      prefs.FindStringByDottedPath("[virtual]:browser.version");

  // Assert
  ASSERT_TRUE(version);
  EXPECT_EQ(*version, version_info::GetVersionNumber());
}

TEST_F(VirtualPrefProviderTest, BrowserMajorVersion) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  std::optional<int> major_version =
      prefs.FindIntByDottedPath("[virtual]:browser.major_version");

  // Assert
  EXPECT_EQ(major_version, version_info::GetVersion().components()[0]);
}

TEST_F(VirtualPrefProviderTest, BrowserMinorVersion) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  std::optional<int> minor_version =
      prefs.FindIntByDottedPath("[virtual]:browser.minor_version");

  // Assert
  EXPECT_EQ(minor_version, version_info::GetVersion().components()[1]);
}

TEST_F(VirtualPrefProviderTest, BrowserBuildVersion) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  std::optional<int> build_version =
      prefs.FindIntByDottedPath("[virtual]:browser.build_version");

  // Assert
  EXPECT_EQ(build_version, version_info::GetVersion().components()[2]);
}

TEST_F(VirtualPrefProviderTest, BrowserPatchVersion) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  std::optional<int> patch_version =
      prefs.FindIntByDottedPath("[virtual]:browser.patch_version");

  // Assert
  EXPECT_EQ(patch_version, version_info::GetVersion().components()[3]);
}

TEST_F(VirtualPrefProviderTest, OperatingSystemLocaleLanguage) {
  // Arrange
  const test::ScopedCurrentLanguageCode scoped_current_language_code{"en"};

  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* language = prefs.FindStringByDottedPath(
      "[virtual]:operating_system.locale.language");

  // Assert
  ASSERT_TRUE(language);
  EXPECT_EQ(*language, "en");
}

TEST_F(VirtualPrefProviderTest, OperatingSystemLocaleRegion) {
  // Arrange
  const test::ScopedCurrentCountryCode scoped_current_country_code{"KY"};

  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* region =
      prefs.FindStringByDottedPath("[virtual]:operating_system.locale.region");

  // Assert
  ASSERT_TRUE(region);
  EXPECT_EQ(*region, "KY");
}

TEST_F(VirtualPrefProviderTest, OperatingSystemIsMobilePlatform) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  std::optional<bool> is_mobile_platform = prefs.FindBoolByDottedPath(
      "[virtual]:operating_system.is_mobile_platform");

  // Assert
  ASSERT_TRUE(is_mobile_platform);
}

TEST_F(VirtualPrefProviderTest, OperatingSystemName) {
  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* name =
      prefs.FindStringByDottedPath("[virtual]:operating_system.name");

  // Assert
  ASSERT_TRUE(name);
  EXPECT_EQ(*name, version_info::GetOSType());
}

TEST_F(VirtualPrefProviderTest, SearchEngineDefaultName) {
  // Arrange
  EXPECT_CALL(delegate(), GetDefaultSearchEngineName)
      .WillOnce(testing::Return("Brave"));

  // Act
  const base::Value::Dict prefs = provider().GetVirtualPrefs();
  const std::string* default_search_engine_name =
      prefs.FindStringByDottedPath("[virtual]:search_engine.default_name");

  // Assert
  ASSERT_TRUE(default_search_engine_name);
  EXPECT_EQ(*default_search_engine_name, "Brave");
}

TEST_F(VirtualPrefProviderTest, GetVirtualPrefsWithSkus) {
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
  const base::Value::Dict prefs = provider().GetVirtualPrefs();

  // Assert
  const base::Value::Dict* skus = prefs.FindDict("[virtual]:skus");
  ASSERT_TRUE(skus);

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

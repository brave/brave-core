// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_country.h"

#include "brave/components/brave_search/common/brave_search_url_processor.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search {

class BraveSearchCountryTest : public testing::Test {
 public:
  BraveSearchCountryTest() = default;

 protected:
  void SetUp() override {
    pref_service_.registry()->RegisterIntegerPref("countryid_at_install", 0);
    pref_service_.registry()->RegisterStringPref("brave.search.country", "");
  }

  TestingPrefServiceSimple pref_service_;
};

// Test country ID to country code conversion.
TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_US) {
  // 'U' = 85, 'S' = 83 => 85 * 256 + 83 = 21843
  EXPECT_EQ("US", CountryIdToCountryCode(21843));
}

TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_ZA) {
  // 'Z' = 90, 'A' = 65 => 90 * 256 + 65 = 23105
  EXPECT_EQ("ZA", CountryIdToCountryCode(23105));
}

TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_DE) {
  // 'D' = 68, 'E' = 69 => 68 * 256 + 69 = 17477
  EXPECT_EQ("DE", CountryIdToCountryCode(17477));
}

TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_GB) {
  // 'G' = 71, 'B' = 66 => 71 * 256 + 66 = 18242
  EXPECT_EQ("GB", CountryIdToCountryCode(18242));
}

TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_AU) {
  // 'A' = 65, 'U' = 85 => 65 * 256 + 85 = 16725
  EXPECT_EQ("AU", CountryIdToCountryCode(16725));
}

TEST_F(BraveSearchCountryTest, CountryIdToCountryCode_Invalid) {
  EXPECT_EQ("", CountryIdToCountryCode(0));
  EXPECT_EQ("", CountryIdToCountryCode(-1));
}

// Test GetBraveSearchCountryCode with user preference override.
TEST_F(BraveSearchCountryTest, UserPreferenceOverridesCountryId) {
  // Set countryid_at_install to US.
  pref_service_.SetInteger("countryid_at_install", 21843);
  // But user prefers South Africa.
  pref_service_.SetString("brave.search.country", "ZA");

  EXPECT_EQ("ZA", GetBraveSearchCountryCode(&pref_service_));
}

// Test GetBraveSearchCountryCode with countryid_at_install.
TEST_F(BraveSearchCountryTest, CountryIdUsedWhenNoUserPref) {
  // Set countryid_at_install to ZA (23105).
  pref_service_.SetInteger("countryid_at_install", 23105);

  EXPECT_EQ("ZA", GetBraveSearchCountryCode(&pref_service_));
}

// Test URL processing with country placeholder.
TEST_F(BraveSearchCountryTest, ProcessBraveSearchUrl_WithCountry) {
  pref_service_.SetInteger("countryid_at_install", 23105);  // ZA

  std::string url =
      "https://search.brave.com/search?q=test&source=desktop"
      "&country={brave:country}";
  std::string result = ProcessBraveSearchUrl(url, &pref_service_);
  EXPECT_EQ(
      "https://search.brave.com/search?q=test&source=desktop&country=ZA",
      result);
}

// Test URL processing when country cannot be determined.
TEST_F(BraveSearchCountryTest, ProcessBraveSearchUrl_NoCountry) {
  // No country info set - countryid_at_install is 0 and no user pref.
  std::string url =
      "https://search.brave.com/search?q=test&source=desktop"
      "&country={brave:country}";
  std::string result = ProcessBraveSearchUrl(url, &pref_service_);
  // The &country= param should be removed entirely.
  EXPECT_EQ("https://search.brave.com/search?q=test&source=desktop", result);
}

// Test URL processing with no placeholder (non-Brave search).
TEST_F(BraveSearchCountryTest, ProcessBraveSearchUrl_NoPlaceholder) {
  std::string url = "https://duckduckgo.com/?q=test&t=brave";
  std::string result = ProcessBraveSearchUrl(url, &pref_service_);
  EXPECT_EQ(url, result);
}

// Test URL processing with null prefs falls back to locale.
TEST_F(BraveSearchCountryTest, ProcessBraveSearchUrl_NullPrefs) {
  std::string url =
      "https://search.brave.com/search?q=test&source=desktop"
      "&country={brave:country}";
  // Should not crash with null prefs.
  std::string result = ProcessBraveSearchUrl(url, nullptr);
  // Result should either have a country code from locale or have the param
  // removed. Either way, the placeholder should not remain.
  EXPECT_EQ(std::string::npos, result.find("{brave:country}"));
}

}  // namespace brave_search

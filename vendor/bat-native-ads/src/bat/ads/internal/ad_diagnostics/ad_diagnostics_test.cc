/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=AdDiagnosticsTest.*

namespace ads {

class AdDiagnosticsTest : public UnitTestBase {
 protected:
  AdDiagnosticsTest() = default;

  ~AdDiagnosticsTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(AdDiagnosticsTest, CheckAdsInitialized) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ("false",
              GetDiagnosticsEntry(*json_value, kDiagnosticsAdsEnabled));
    EXPECT_EQ("false",
              GetDiagnosticsEntry(*json_value, kDiagnosticsAdsInitialized));
    EXPECT_EQ("en-US", GetDiagnosticsEntry(*json_value, kDiagnosticsLocale));
  });

  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);
  InitializeAds();

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ("true", GetDiagnosticsEntry(*json_value, kDiagnosticsAdsEnabled));
    EXPECT_EQ("true",
              GetDiagnosticsEntry(*json_value, kDiagnosticsAdsInitialized));
    EXPECT_EQ("en-US", GetDiagnosticsEntry(*json_value, kDiagnosticsLocale));
  });
}

TEST_F(AdDiagnosticsTest, CatalogPrefs) {
  // Arrange
  InitializeAds();
  const std::string catalog_id = "catalog_id";
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId, catalog_id);

  const base::Time catalog_update_time = base::Time::Now();
  AdsClientHelper::Get()->SetInt64Pref(
      prefs::kCatalogLastUpdated,
      static_cast<int64_t>(catalog_update_time.ToDoubleT()));
  const std::string update_time_str =
      base::UTF16ToUTF8(base::TimeFormatShortDateAndTime(catalog_update_time));

  // Act & Assert
  GetAds()->GetAdDiagnostics([&catalog_id, &update_time_str](
                                 const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ(catalog_id,
              GetDiagnosticsEntry(*json_value, kDiagnosticsCatalogId));
    EXPECT_EQ(update_time_str,
              GetDiagnosticsEntry(*json_value, kDiagnosticsCatalogLastUpdated));
  });
}

TEST_F(AdDiagnosticsTest, LastUnidleTimestamp) {
  // Arrange
  InitializeAds();

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    const auto entry =
        GetDiagnosticsEntry(*json_value, kDiagnosticsLastUnIdleTimestamp);
    ASSERT_TRUE(entry.has_value());
    EXPECT_TRUE(entry->empty());
  });

  // Arrange
  GetAds()->OnUnIdle(/* idle_time */ 1, /* was_locked */ false);

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    const auto entry =
        GetDiagnosticsEntry(*json_value, kDiagnosticsLastUnIdleTimestamp);
    ASSERT_TRUE(entry.has_value());
    EXPECT_FALSE(entry->empty());
  });
}

}  // namespace ads

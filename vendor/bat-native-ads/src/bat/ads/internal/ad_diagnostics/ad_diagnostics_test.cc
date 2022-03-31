/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"

#include <string>

#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"
#include "bat/ads/internal/ad_diagnostics/ads_enabled_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/catalog_id_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/catalog_last_updated_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/last_unidle_timestamp_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/locale_ad_diagnostics_entry.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=AdDiagnosticsTest.*

namespace ads {

class AdDiagnosticsTest : public UnitTestBase {
 protected:
  AdDiagnosticsTest() {}

  ~AdDiagnosticsTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(AdDiagnosticsTest, AdsEnabledAndLocale) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ("false",
              GetDiagnosticsValueByKey(
                  *json_value, AdsEnabledAdDiagnosticsEntry().GetKey()));
    EXPECT_EQ("en-US", GetDiagnosticsValueByKey(
                           *json_value, LocaleAdDiagnosticsEntry().GetKey()));
  });

  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);
  InitializeAds();

  // Act & Assert
  GetAds()->GetAdDiagnostics([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ("true",
              GetDiagnosticsValueByKey(
                  *json_value, AdsEnabledAdDiagnosticsEntry().GetKey()));
  });
}

TEST_F(AdDiagnosticsTest, CatalogPrefs) {
  // Arrange
  InitializeAds();
  const std::string catalog_id = "catalog_id";
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId, catalog_id);

  const double catalog_last_updated_timestamp = NowAsTimestamp();
  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        catalog_last_updated_timestamp);

  const base::Time catalog_last_updated_time =
      TimestampToTime(catalog_last_updated_timestamp);

  const std::string formatted_catalog_last_updated_time = base::UTF16ToUTF8(
      base::TimeFormatShortDateAndTime(catalog_last_updated_time));

  // Act & Assert
  GetAds()->GetAdDiagnostics([&catalog_id,
                              &formatted_catalog_last_updated_time](
                                 const bool success, const std::string& json) {
    ASSERT_TRUE(success);
    const auto json_value = base::JSONReader::Read(json);
    ASSERT_TRUE(json_value);

    EXPECT_EQ(catalog_id,
              GetDiagnosticsValueByKey(*json_value,
                                       CatalogIdAdDiagnosticsEntry().GetKey()));
    EXPECT_EQ(
        formatted_catalog_last_updated_time,
        GetDiagnosticsValueByKey(
            *json_value, CatalogLastUpdatedAdDiagnosticsEntry().GetKey()));
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

    const auto entry = GetDiagnosticsValueByKey(
        *json_value, LastUnIdleTimestampAdDiagnosticsEntry().GetKey());
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

    const auto entry = GetDiagnosticsValueByKey(
        *json_value, LastUnIdleTimestampAdDiagnosticsEntry().GetKey());
    ASSERT_TRUE(entry.has_value());
    EXPECT_FALSE(entry->empty());
  });
}

}  // namespace ads

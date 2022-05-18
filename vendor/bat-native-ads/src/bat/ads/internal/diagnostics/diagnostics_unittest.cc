/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostics.h"

#include <string>

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsDiagnosticsTest : public UnitTestBase {
 protected:
  BatAdsDiagnosticsTest() = default;

  ~BatAdsDiagnosticsTest() override = default;
};

TEST_F(BatAdsDiagnosticsTest, Diagnostics) {
  // Arrange
  AdvanceClock(
      TimeFromString("Wed, 18 Nov 1970 12:34:56", /* is_local */ false));

  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  MockLocaleHelper(locale_helper_mock_, "en-KY");

  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId,
                                        "da5dd0e8-71e9-4607-a45b-13e28b607a81");
  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        NowAsTimestamp());

  SetLastUnIdleTimeDiagnosticEntry();

  // Act
  Diagnostics::Get()->Get([](const bool success, const std::string& json) {
    ASSERT_TRUE(success);

    const std::string expected_json =
        R"([{"name":"Enabled","value":"true"},{"name":"Locale","value":"en-KY"},{"name":"Catalog ID","value":"da5dd0e8-71e9-4607-a45b-13e28b607a81"},{"name":"Catalog last updated","value":"1970-11-18T12:34:56.000Z"},{"name":"Last unidle time","value":"1970-11-18T12:34:56.000Z"}])";
    EXPECT_EQ(expected_json, json);
  });

  // Assert
}

}  // namespace ads

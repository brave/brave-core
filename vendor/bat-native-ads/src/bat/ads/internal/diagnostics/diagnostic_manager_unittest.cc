/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostic_manager.h"

#include "absl/types/optional.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/sys_info.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsDiagnosticManagerTest : public UnitTestBase {};

TEST_F(BatAdsDiagnosticManagerTest, DiagnosticManager) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("Wed, 18 Nov 1970 12:34:56", /*is_local*/ true));

  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  SetCatalogId("da5dd0e8-71e9-4607-a45b-13e28b607a81");
  SetCatalogLastUpdated(Now());

  AdvanceClockTo(TimeFromString("Mon, 8 Jul 1996 09:25:00", /*is_local*/ true));

  SetLastUnIdleTimeDiagnosticEntry();

  // Act
  DiagnosticManager::GetInstance()->GetDiagnostics(
      base::BindOnce([](absl::optional<base::Value::List> list) {
        // Assert
        ASSERT_TRUE(list);

        const base::Value expected_list = base::test::ParseJson(
            R"([
                {
                  "name": "Device Id",
                  "value": "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e"
                },
                {
                  "name": "Enabled",
                  "value": "true"
                },
                {
                  "name": "Locale",
                  "value": "en_KY"
                },
                {
                  "name": "Catalog ID",
                  "value": "da5dd0e8-71e9-4607-a45b-13e28b607a81"
                },
                {
                  "name": "Catalog last updated",
                  "value": "Wednesday, November 18, 1970 at 12:34:56\u202fPM"
                },
                {
                  "name": "Last unidle time",
                  "value": "Monday, July 8, 1996 at 9:25:00\u202fAM"
                }
              ])");
        ASSERT_TRUE(expected_list.is_list());

        EXPECT_EQ(expected_list, list);
      }));
}

}  // namespace ads

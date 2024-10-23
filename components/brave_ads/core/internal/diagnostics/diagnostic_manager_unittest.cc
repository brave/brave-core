/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"

#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry_util.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsDiagnosticManagerTest : public test::TestBase {};

TEST_F(BraveAdsDiagnosticManagerTest, DiagnosticManager) {
  // Arrange
  test::MockDeviceId();

  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"en_KY"};

  AdvanceClockTo(test::TimeFromString("Wed, 18 Nov 1970 12:34:56"));

  SetCatalogId(test::kCatalogId);
  SetCatalogLastUpdated(test::Now());

  AdvanceClockTo(
      test::TimeFromString("Fri, 16 Mar 2012 06:23:00"));  // Hello Phoebe!!!

  SetLastUnIdleTimeDiagnosticEntry(test::Now());

  // Act & Assert
  const base::Value::List expected_diagnostics = base::test::ParseJsonList(
      R"(
          [
            {
              "name": "Device Id",
              "value": "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e"
            },
            {
              "name": "Opted into Brave News ads",
              "value": "true"
            },
            {
              "name": "Opted into new tab page ads",
              "value": "true"
            },
            {
              "name": "Opted into notification ads",
              "value": "true"
            },
            {
              "name": "Opted into search result ads",
              "value": "true"
            },
            {
              "name": "Locale",
              "value": "en_KY"
            },
            {
              "name": "Catalog ID",
              "value": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
            },
            {
              "name": "Catalog last updated",
              "value": "Wednesday, November 18, 1970 at 12:34:56\u202fPM"
            },
            {
              "name": "Last unidle time",
              "value": "Friday, March 16, 2012 at 6:23:00\u202fAM"
            }
          ])");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetDiagnostics(callback.Get());
}

}  // namespace brave_ads

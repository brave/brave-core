/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"

#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/catalog/test/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_environment_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry_util.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsDiagnosticManagerTest : public test::TestBase {};

TEST_F(BraveAdsDiagnosticManagerTest, DiagnosticManager) {
  // Arrange
  test::SetUpDeviceId();

  fake_locale_.SetCountryCode("KY");

  AdvanceClockTo(test::TimeFromString("Wed, 18 Nov 1970 12:34:56"));

  SetCatalogId(test::kCatalogId);
  SetCatalogLastUpdated(test::Now());

  AdvanceClockTo(
      test::TimeFromString("Fri, 16 Mar 2012 06:23:00"));  // Hello Phoebe!!!

  SetLastUnIdleTimeDiagnosticEntry(test::Now());

  // Act & Assert
  const base::ListValue expected_diagnostics = base::test::ParseJsonList(
      R"JSON(
          [
            {
              "name": "Device ID",
              "value": "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e"
            },
            {
              "name": "Opted into new tab page ads",
              "value": "true"
            },
            {
              "name": "Notification ads enabled",
              "value": "true"
            },
            {
              "name": "Opted into search result ads",
              "value": "true"
            },
            {
              "name": "Language",
              "value": "en"
            },
            {
              "name": "Country",
              "value": "KY"
            },
            {
              "name": "Catalog ID",
              "value": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
            },
            {
              "name": "Catalog last updated",
              "value": "Wednesday, November 18, 1970 at 12:34:56\u202fPM (15092 days overdue)"
            },
            {
              "name": "Last unidle time",
              "value": "Friday, March 16, 2012 at 6:23:00\u202fAM (less than a minute ago)"
            }
          ])JSON");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetDiagnostics(callback.Get());
}

TEST_F(BraveAdsDiagnosticManagerTest, GetRewardsDiagnostics) {
  // Act & Assert
  const base::ListValue expected_diagnostics = base::test::ParseJsonList(
      R"JSON(
          [
            {
              "name": "Wallet valid",
              "value": "N/A"
            },
            {
              "name": "Connected",
              "value": "true"
            },
            {
              "name": "Issuers valid",
              "value": "N/A"
            }
          ])JSON");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetRewardsDiagnostics(callback.Get());
}

TEST_F(BraveAdsDiagnosticManagerTest, GetStorageDiagnostics) {
  // Act & Assert
  const base::ListValue expected_diagnostics = base::test::ParseJsonList(
      R"JSON(
          [
            {
              "name": "Schema version",
              "value": "60"
            },
            {
              "name": "Last migration failure reason",
              "value": "None"
            }
          ])JSON");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetStorageDiagnostics(callback.Get());
}

TEST_F(BraveAdsDiagnosticManagerTest, GetResourcesDiagnostics) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("Wed, 18 Nov 1970 12:34:56"));

  SetCatalogId(test::kCatalogId);
  SetCatalogLastUpdated(test::Now());

  AdvanceClockTo(
      test::TimeFromString("Fri, 16 Mar 2012 06:23:00"));  // Hello Phoebe!!!

  // Act & Assert
  const base::ListValue expected_diagnostics = base::test::ParseJsonList(
      R"JSON(
          [
            {
              "name": "Catalog Version",
              "value": "N/A"
            },
            {
              "name": "New Tab Page Ads Schema Version",
              "value": "N/A"
            },
            {
              "name": "Catalog next update",
              "value": "Wednesday, November 18, 1970 at 2:34:56 PM (15093 days ago)"
            },
            {
              "name": "Text classification resource",
              "value": "Not loaded"
            },
            {
              "name": "Purchase intent resource",
              "value": "Not loaded"
            },
            {
              "name": "Anti targeting resource",
              "value": "Not loaded"
            }
          ])JSON");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetResourcesDiagnostics(callback.Get());
}

TEST_F(BraveAdsDiagnosticManagerTest, GetPermissionRulesDiagnostics) {
  // Act & Assert
  const base::ListValue expected_diagnostics = base::test::ParseJsonList(
      R"JSON(
          [
            {
              "name": "Catalog permission",
              "value": "false"
            },
            {
              "name": "Network connection permission",
              "value": "true"
            },
            {
              "name": "Browser is active permission",
              "value": "false"
            },
            {
              "name": "Full screen mode permission",
              "value": "true"
            },
            {
              "name": "Media permission",
              "value": "true"
            },
            {
              "name": "Do not disturb permission",
              "value": "true"
            },
            {
              "name": "Issuers permission",
              "value": "false"
            },
            {
              "name": "Confirmation tokens permission",
              "value": "false"
            },
            {
              "name": "User activity permission",
              "value": "true"
            },
            {
              "name": "Command line permission",
              "value": "true"
            },
            {
              "name": "Can show notifications permission",
              "value": "true"
            }
          ])JSON");

  base::MockCallback<GetDiagnosticsCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_diagnostics))));
  DiagnosticManager::GetInstance().GetPermissionRulesDiagnostics(
      callback.Get());
}

TEST_F(BraveAdsDiagnosticManagerTest, GetConditionMatchers) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  test::RegisterProfileStringPref("bar", "not_a_number");
  test::RegisterProfileIntegerPref("baz", 3);

  CreativeNewTabPageAdInfo creative_ad = test::BuildCreativeNewTabPageAd(
      CreativeNewTabPageAdWallpaperType::kImage, /*use_random_uuids=*/false);
  creative_ad.condition_matchers = {
      {"foo", "[R=]:5"},  // Matches.
      {"foo", "[R=]:4"},  // Does not match.
      // Operand resolved from another pref, shown alongside for display.
      {"foo", "[R>]:baz"},
      {"bar", "[R=]:5"},              // Invalid: non-numeric matched value.
      {"unknown_pref_path", "baz"}};  // Unknown current value.
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act & Assert
  //
  // `ConditionMatcherMap` is a `std::multimap` keyed by pref path, so entries
  // come back sorted alphabetically by "Pref Path" ("bar" < "foo" <
  // "unknown_pref_path"), with the two "foo" entries in insertion order.
  const base::ListValue expected_condition_matchers =
      base::test::ParseJsonList(absl::StrFormat(
          R"JSON(
          [
            {
              "Creative Instance ID": "%s",
              "Pref Path": "bar",
              "Condition": "[R=]:5",
              "Current Value": "not_a_number",
              "Matches": "Invalid"
            },
            {
              "Creative Instance ID": "%s",
              "Pref Path": "foo",
              "Condition": "[R=]:5",
              "Current Value": "5",
              "Matches": "Yes"
            },
            {
              "Creative Instance ID": "%s",
              "Pref Path": "foo",
              "Condition": "[R=]:4",
              "Current Value": "5",
              "Matches": "No"
            },
            {
              "Creative Instance ID": "%s",
              "Pref Path": "foo",
              "Condition": "[R>]:baz (3)",
              "Current Value": "5",
              "Matches": "Yes"
            },
            {
              "Creative Instance ID": "%s",
              "Pref Path": "unknown_pref_path",
              "Condition": "baz",
              "Current Value": "Unknown",
              "Matches": "No"
            }
          ])JSON",
          creative_ad.creative_instance_id.c_str(),
          creative_ad.creative_instance_id.c_str(),
          creative_ad.creative_instance_id.c_str(),
          creative_ad.creative_instance_id.c_str(),
          creative_ad.creative_instance_id.c_str()));

  base::test::TestFuture<bool, base::ListValue> test_future;
  DiagnosticManager::GetConditionMatchers(
      test_future.GetCallback<bool, base::ListValue>());
  const auto [success, condition_matchers] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_EQ(expected_condition_matchers, condition_matchers);
}

}  // namespace brave_ads

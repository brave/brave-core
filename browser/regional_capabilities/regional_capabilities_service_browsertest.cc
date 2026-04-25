/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/regional_capabilities/regional_capabilities_service.h"

#include "base/check_deref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/regional_capabilities/regional_capabilities_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/country_codes/country_codes.h"
#include "components/regional_capabilities/regional_capabilities_country_id.h"
#include "components/variations/variations_switches.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace regional_capabilities {

class RegionalCapabilitiesServiceBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_NE("FR", country_codes::GetCurrentCountryID().CountryCode());
    command_line->AppendSwitchASCII(
        variations::switches::kVariationsOverrideCountry, "fr");
  }
};

// Make sure that regional capabilities service retrieves locale from
// the device (`country_codes::GetCurrentCountryID()`) rather than from
// the variations service on all desktop platforms.
IN_PROC_BROWSER_TEST_F(RegionalCapabilitiesServiceBrowserTest, GetCountryId) {
  auto& service = CHECK_DEREF(
      RegionalCapabilitiesServiceFactory::GetForProfile(browser()->profile()));

  country_codes::CountryId expected_country_id =
      country_codes::GetCurrentCountryID();
  country_codes::CountryId actual_country_id =
      service.GetCountryId().GetForTesting();
  EXPECT_EQ(expected_country_id, actual_country_id)
      << "Country id retrieved by the regional capabilities service doesn't "
         "match the device locale (actual = "
      << actual_country_id.CountryCode()
      << " vs. expected = " << expected_country_id.CountryCode() << ")";
}

}  // namespace regional_capabilities

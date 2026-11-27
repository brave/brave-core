/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_country_code.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_command_line.h"
#include "base/test/task_environment.h"
#include "components/country_codes/country_codes.h"
#include "components/metrics/metrics_state_manager.h"
#include "components/metrics/test/test_enabled_state_provider.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/test_variations_service.h"
#include "components/variations/variations_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sponsored_content {

class SponsoredContentCountryCodeTest : public testing::Test {
 public:
  SponsoredContentCountryCodeTest() {
    variations::TestVariationsService::RegisterPrefs(local_state_.registry());
    metrics_state_manager_ = metrics::MetricsStateManager::Create(
        &local_state_, &enabled_state_provider_,
        /*backup_registry_key=*/std::wstring(),
        /*user_data_dir=*/base::FilePath());
    variations_service_ = std::make_unique<variations::TestVariationsService>(
        &local_state_, metrics_state_manager_.get());
  }

 protected:
  // `TestVariationsService` needs a `ThreadPool` instance.
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  metrics::TestEnabledStateProvider enabled_state_provider_{/*consent=*/false,
                                                            /*enabled=*/false};
  std::unique_ptr<metrics::MetricsStateManager> metrics_state_manager_;
  std::unique_ptr<variations::TestVariationsService> variations_service_;
};

TEST_F(SponsoredContentCountryCodeTest,
       FallsBackToIsoFormattedCountryCodeWhenVariationsServiceIsNull) {
  const std::string expected_country_code = base::ToUpperASCII(
      std::string(country_codes::GetCurrentCountryID().CountryCode()));
  EXPECT_EQ(expected_country_code,
            GetVariationsCountryCode(/*variations_service=*/nullptr));
}

TEST_F(SponsoredContentCountryCodeTest,
       ReturnsCommandLineOverrideCountryInIsoFormat) {
  base::test::ScopedCommandLine scoped_command_line;
  scoped_command_line.GetProcessCommandLine()->AppendSwitchASCII(
      variations::switches::kVariationsOverrideCountry, "gb");
  ASSERT_EQ("gb", variations_service_->GetLatestCountry());

  EXPECT_EQ("GB", GetVariationsCountryCode(variations_service_.get()));
}

// The command-line override above takes priority over any stored seed, so
// this covers the other source `GetLatestCountry` can return from.
TEST_F(SponsoredContentCountryCodeTest,
       ReturnsStoredSeedCountryInIsoFormatWhenAvailable) {
  local_state_.SetString(variations::prefs::kVariationsCountry, "fr");
  ASSERT_EQ("fr", variations_service_->GetLatestCountry());

  EXPECT_EQ("FR", GetVariationsCountryCode(variations_service_.get()));
}

}  // namespace sponsored_content

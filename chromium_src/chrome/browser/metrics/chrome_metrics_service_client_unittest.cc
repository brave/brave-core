/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */


// Disable the original tests we're going to override.
#define TestRegisterUKMProviders DISABLED_TestRegisterUKMProviders
#define TestRegisterMetricsServiceProviders \
  DISABLED_TestRegisterMetricsServiceProviders

#include "src/chrome/browser/metrics/chrome_metrics_service_client_unittest.cc"

#undef TestRegisterMetricsServiceProviders
#undef TestRegisterUKMProviders

TEST_F(ChromeMetricsServiceClientTest, BraveTestRegisterUKMProviders) {
  std::unique_ptr<ChromeMetricsServiceClient> chrome_metrics_service_client =
      TestChromeMetricsServiceClient::Create(metrics_state_manager_.get(),
                                             synthetic_trial_registry_.get());
  size_t observed_count = chrome_metrics_service_client->GetUkmService()
                              ->metrics_providers_.GetProviders()
                              .size();
  // In Brave, we expect 0 UKM providers regardless of feature flag
  EXPECT_EQ(0ul, observed_count);
}

TEST_F(ChromeMetricsServiceClientTest, BraveRegisterMetricsServiceProviders) {
  std::unique_ptr<TestChromeMetricsServiceClient>
      chrome_metrics_service_client = TestChromeMetricsServiceClient::Create(
          metrics_state_manager_.get(), synthetic_trial_registry_.get());

  // In Brave, we expect only 2 metrics providers (the ones added in the
  // MetricsService constructor)
  EXPECT_EQ(2ul, chrome_metrics_service_client->GetMetricsService()
                     ->GetDelegatingProviderForTesting()
                     ->GetProviders()
                     .size());
}

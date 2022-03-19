/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"

#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "components/metrics/enabled_state_provider.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ChromeMetricsServicesManagerClient, MetricsReportingDisabled) {
  TestingPrefServiceSimple local_state;
  metrics::RegisterMetricsReportingStatePrefs(local_state.registry());
  local_state.registry()->RegisterBooleanPref(
      metrics::prefs::kMetricsReportingEnabled, true);

  ChromeMetricsServicesManagerClient client(&local_state);
  const metrics::EnabledStateProvider& provider =
      client.GetEnabledStateProviderForTesting();

  // Reporting should never be enabled
  EXPECT_FALSE(provider.IsReportingEnabled());
}

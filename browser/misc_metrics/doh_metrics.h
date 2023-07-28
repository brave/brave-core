/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_DOH_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_DOH_METRICS_H_

#include "base/timer/timer.h"

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_change_registrar.h"
#include "services/network/public/mojom/network_service.mojom.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

extern const char kAutoSecureRequestsHistogramName[];
extern const char kSecureDnsSettingHistogramName[];

// DNS-over-HTTPS metrics.
class DohMetrics : public base::SupportsWeakPtr<DohMetrics> {
 public:
  explicit DohMetrics(PrefService* local_state);
  ~DohMetrics();

  DohMetrics(const DohMetrics&) = delete;
  DohMetrics& operator=(const DohMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  friend class DohMetricsTest;
  FRIEND_TEST_ALL_PREFIXES(DohMetricsTest, AutoSecureRequests);

  void HandleDnsOverHttpsMode();

  void StartAutoUpgradeInitTimer();
  void OnAutoUpgradeInitTimer();

  void StartAutoUpgradeReportTimer();
  void OnAutoUpgradeReportTimer();

  void StopListeningToDnsRequests();
  void OnDnsRequestCounts(network::mojom::DnsRequestCountsPtr counts);

  WeeklyStorage total_request_storage_;
  WeeklyStorage upgraded_request_storage_;

  PrefChangeRegistrar pref_change_registrar_;
  raw_ptr<PrefService> local_state_;

  base::OneShotTimer init_timer_;
  base::RepeatingTimer report_interval_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_DOH_METRICS_H_

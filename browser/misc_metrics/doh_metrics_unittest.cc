/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/doh_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class DohMetricsTest : public testing::Test {
 public:
  DohMetricsTest() = default;

 protected:
  void SetUp() override {
    auto* registry = local_state_.registry();
    registry->RegisterStringPref(prefs::kDnsOverHttpsMode, "");
    DohMetrics::RegisterPrefs(registry);
    doh_metrics_ = std::make_unique<DohMetrics>(&local_state_);
  }

  void TearDown() override { doh_metrics_.reset(); }

  std::unique_ptr<DohMetrics> doh_metrics_;
  base::HistogramTester histogram_tester_;
  TestingPrefServiceSimple local_state_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(DohMetricsTest, SecureSetting) {
  histogram_tester_.ExpectUniqueSample(kSecureDnsSettingHistogramName, 1, 1);

  local_state_.SetString(prefs::kDnsOverHttpsMode, "off");
  histogram_tester_.ExpectBucketCount(kSecureDnsSettingHistogramName,
                                      INT_MAX - 1, 1);

  local_state_.SetString(prefs::kDnsOverHttpsMode, "secure");
  histogram_tester_.ExpectBucketCount(kSecureDnsSettingHistogramName, 2, 1);

  local_state_.SetString(prefs::kDnsOverHttpsMode, "automatic");
  histogram_tester_.ExpectBucketCount(kSecureDnsSettingHistogramName, 1, 2);
}

TEST_F(DohMetricsTest, AutoSecureRequests) {
  histogram_tester_.ExpectTotalCount(kAutoSecureRequestsHistogramName, 0);

  // Should not record if percentage is 0%
  doh_metrics_->OnDnsRequestCounts(network::mojom::DnsRequestCounts::New(1, 0));
  histogram_tester_.ExpectUniqueSample(kAutoSecureRequestsHistogramName,
                                       INT_MAX - 1, 1);

  doh_metrics_->OnDnsRequestCounts(
      network::mojom::DnsRequestCounts::New(11, 11));
  histogram_tester_.ExpectBucketCount(kAutoSecureRequestsHistogramName, 3, 1);

  doh_metrics_->OnDnsRequestCounts(network::mojom::DnsRequestCounts::New(6, 1));
  histogram_tester_.ExpectBucketCount(kAutoSecureRequestsHistogramName, 2, 1);

  doh_metrics_->OnDnsRequestCounts(network::mojom::DnsRequestCounts::New(7, 0));
  histogram_tester_.ExpectBucketCount(kAutoSecureRequestsHistogramName, 1, 1);

  doh_metrics_->OnDnsRequestCounts(
      network::mojom::DnsRequestCounts::New(250, 0));
  histogram_tester_.ExpectBucketCount(kAutoSecureRequestsHistogramName, 0, 1);

  local_state_.SetString(prefs::kDnsOverHttpsMode, "secure");
  histogram_tester_.ExpectBucketCount(kAutoSecureRequestsHistogramName,
                                      INT_MAX - 1, 2);
  histogram_tester_.ExpectTotalCount(kAutoSecureRequestsHistogramName, 6);
}

}  // namespace misc_metrics

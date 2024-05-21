/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/doh_metrics.h"

#include <algorithm>
#include <string>

#include "base/metrics/histogram_macros.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/network_service_instance.h"
#include "net/base/features.h"

namespace misc_metrics {

using net::DohFallbackEndpointType;
using net::features::kBraveFallbackDoHProvider;
using net::features::kBraveFallbackDoHProviderEndpoint;

namespace {

const int kAutoSecureRequestsBuckets[] = {5, 50, 90};

const char kDohModeAutomatic[] = "automatic";
const char kDohModeSecure[] = "secure";
const base::TimeDelta kAutoSecureInitDelay = base::Seconds(6);
const base::TimeDelta kAutoSecureReportInterval = base::Seconds(20);

constexpr char kQuad9Suffix[] = ".Quad9";
constexpr char kWikimediaSuffix[] = ".Wikimedia";
constexpr char kCloudflareSuffix[] = ".Cloudflare";

std::string GetAutoSecureRequestsHistogramName() {
  std::string histogram_name = kAutoSecureRequestsHistogramName;

  if (base::FeatureList::IsEnabled(net::features::kBraveFallbackDoHProvider)) {
    auto endpoint_type = kBraveFallbackDoHProviderEndpoint.Get();

    switch (endpoint_type) {
      case DohFallbackEndpointType::kQuad9:
        histogram_name += kQuad9Suffix;
        break;
      case DohFallbackEndpointType::kWikimedia:
        histogram_name += kWikimediaSuffix;
        break;
      case DohFallbackEndpointType::kCloudflare:
        histogram_name += kCloudflareSuffix;
        break;
      default:
        break;
    }
  }
  return histogram_name;
}

}  // namespace

DohMetrics::DohMetrics(PrefService* local_state) : local_state_(local_state) {
  int current_fallback_int = static_cast<int>(DohFallbackEndpointType::kNone);
  if (base::FeatureList::IsEnabled(net::features::kBraveFallbackDoHProvider)) {
    current_fallback_int =
        static_cast<int>(kBraveFallbackDoHProviderEndpoint.Get());
  }
  int last_fallback_int = local_state->GetInteger(kMiscMetricsLastDohFallback);
  if (current_fallback_int != last_fallback_int) {
    local_state->SetInteger(kMiscMetricsLastDohFallback, current_fallback_int);
    if (last_fallback_int != -1) {
      // New users, and users that upgraded to a client version
      // that introduced this pref should not clear the collected stats.
      // Only users that we're suddenly included in a DoH fallback study should
      // clear existing stats, so we can collect fresh stats.
      local_state->ClearPref(kMiscMetricsTotalDnsRequestStorage);
      local_state->ClearPref(kMiscMetricsUpgradedDnsRequestStorage);
    }
  }

  total_request_storage_ = std::make_unique<WeeklyStorage>(
      local_state, kMiscMetricsTotalDnsRequestStorage);
  upgraded_request_storage_ = std::make_unique<WeeklyStorage>(
      local_state, kMiscMetricsUpgradedDnsRequestStorage);

  pref_change_registrar_.Init(local_state_);
  pref_change_registrar_.Add(
      prefs::kDnsOverHttpsMode,
      base::BindRepeating(&DohMetrics::HandleDnsOverHttpsMode,
                          base::Unretained(this)));
  HandleDnsOverHttpsMode();
}

DohMetrics::~DohMetrics() {
  pref_change_registrar_.Remove(prefs::kDnsOverHttpsMode);
}

void DohMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsTotalDnsRequestStorage);
  registry->RegisterListPref(kMiscMetricsUpgradedDnsRequestStorage);
  registry->RegisterIntegerPref(kMiscMetricsLastDohFallback, -1);
}

void DohMetrics::HandleDnsOverHttpsMode() {
  std::string mode = local_state_->GetString(prefs::kDnsOverHttpsMode);
  int setting_answer = INT_MAX - 1;
  if (mode.empty() || mode == kDohModeAutomatic) {
    setting_answer = 1;
    StartAutoUpgradeInitTimer();
  } else if (mode == kDohModeSecure) {
    setting_answer = 2;
    StopListeningToDnsRequests();
  } else {
    StopListeningToDnsRequests();
  }
  UMA_HISTOGRAM_EXACT_LINEAR(kSecureDnsSettingHistogramName, setting_answer, 3);
}

void DohMetrics::OnDnsRequestCounts(
    network::mojom::DnsRequestCountsPtr counts) {
  std::string mode = local_state_->GetString(prefs::kDnsOverHttpsMode);
  if (!mode.empty() && mode != kDohModeAutomatic) {
    return;
  }
  if (counts->upgraded_count > 0) {
    upgraded_request_storage_->AddDelta(counts->upgraded_count);
  }
  if (counts->total_count > 0) {
    total_request_storage_->AddDelta(counts->total_count);
  }

  std::string histogram_name = GetAutoSecureRequestsHistogramName();

  double percentage =
      static_cast<double>(upgraded_request_storage_->GetWeeklySum()) /
      std::max(total_request_storage_->GetWeeklySum(), uint64_t(1u)) * 100.0;
  if (percentage == 0.0) {
    UMA_HISTOGRAM_EXACT_LINEAR(histogram_name, INT_MAX - 1, 4);
    return;
  }

  p3a_utils::RecordToHistogramBucket(histogram_name.c_str(),
                                     kAutoSecureRequestsBuckets,
                                     static_cast<int>(percentage));
}

void DohMetrics::StartAutoUpgradeInitTimer() {
  init_timer_.Start(FROM_HERE, kAutoSecureInitDelay, this,
                    &DohMetrics::OnAutoUpgradeInitTimer);
}

void DohMetrics::OnAutoUpgradeInitTimer() {
  // Call `GetCountsAndReset` to reset the internal counts, so we can
  // get fresh results. We delay the initial report so that we can give the
  // DNS resolver a chance to load the secure DNS information from the current
  // provider.
  content::GetNetworkService()->GetDnsRequestCountsAndReset(base::DoNothing());
  report_interval_timer_.Start(FROM_HERE, kAutoSecureReportInterval, this,
                               &DohMetrics::OnAutoUpgradeReportTimer);
}

void DohMetrics::OnAutoUpgradeReportTimer() {
  content::GetNetworkService()->GetDnsRequestCountsAndReset(base::BindOnce(
      &DohMetrics::OnDnsRequestCounts, weak_ptr_factory_.GetWeakPtr()));
}

void DohMetrics::StopListeningToDnsRequests() {
  init_timer_.Stop();
  report_interval_timer_.Stop();
  UMA_HISTOGRAM_EXACT_LINEAR(GetAutoSecureRequestsHistogramName(), INT_MAX - 1,
                             4);
}

}  // namespace misc_metrics

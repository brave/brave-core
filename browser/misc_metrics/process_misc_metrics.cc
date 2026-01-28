/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/process_misc_metrics.h"

#include "brave/browser/misc_metrics/doh_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor_impl.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/misc_metrics/default_browser_monitor_delegate_impl.h"
#include "brave/browser/misc_metrics/vertical_tab_metrics.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "brave/components/misc_metrics/new_tab_metrics.h"
#include "brave/components/misc_metrics/split_view_metrics.h"
#else
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#include "brave/components/misc_metrics/tab_metrics.h"
#endif

namespace misc_metrics {

ProcessMiscMetrics::ProcessMiscMetrics(PrefService* local_state) {
#if !BUILDFLAG(IS_ANDROID)
  menu_metrics_ = std::make_unique<MenuMetrics>(local_state);
  new_tab_metrics_ = std::make_unique<NewTabMetrics>(local_state);
  vertical_tab_metrics_ = std::make_unique<VerticalTabMetrics>(local_state);
  split_view_metrics_ = std::make_unique<SplitViewMetrics>(local_state);
  default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>(
      std::make_unique<DefaultBrowserMonitorDelegateImpl>(local_state));
  default_browser_monitor_->Start();
#else
  default_browser_monitor_ = std::make_unique<DefaultBrowserMonitor>();
  privacy_hub_metrics_ = std::make_unique<PrivacyHubMetrics>(local_state);
  tab_metrics_ = std::make_unique<TabMetrics>(local_state);
#endif
  doh_metrics_ = std::make_unique<DohMetrics>(local_state);
  uptime_monitor_ = std::make_unique<UptimeMonitorImpl>(local_state);
  serp_metrics_ = std::make_unique<metrics::SerpMetrics>(local_state);
}

ProcessMiscMetrics::~ProcessMiscMetrics() = default;

#if !BUILDFLAG(IS_ANDROID)
MenuMetrics* ProcessMiscMetrics::menu_metrics() {
  return menu_metrics_.get();
}

NewTabMetrics* ProcessMiscMetrics::new_tab_metrics() {
  return new_tab_metrics_.get();
}

VerticalTabMetrics* ProcessMiscMetrics::vertical_tab_metrics() {
  return vertical_tab_metrics_.get();
}

SplitViewMetrics* ProcessMiscMetrics::split_view_metrics() {
  return split_view_metrics_.get();
}
#else
PrivacyHubMetrics* ProcessMiscMetrics::privacy_hub_metrics() {
  return privacy_hub_metrics_.get();
}

TabMetrics* ProcessMiscMetrics::tab_metrics() {
  return tab_metrics_.get();
}
#endif

DefaultBrowserMonitor* ProcessMiscMetrics::default_browser_monitor() {
  return default_browser_monitor_.get();
}

UptimeMonitorImpl* ProcessMiscMetrics::uptime_monitor() {
  return uptime_monitor_.get();
}

metrics::SerpMetrics* ProcessMiscMetrics::serp_metrics() const {
  return serp_metrics_.get();
}

void ProcessMiscMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  MenuMetrics::RegisterPrefs(registry);
  NewTabMetrics::RegisterPrefs(registry);
  VerticalTabMetrics::RegisterPrefs(registry);
  SplitViewMetrics::RegisterPrefs(registry);
#else
  PrivacyHubMetrics::RegisterPrefs(registry);
  TabMetrics::RegisterPrefs(registry);
#endif
  DohMetrics::RegisterPrefs(registry);
  UptimeMonitorImpl::RegisterPrefs(registry);
}

}  // namespace misc_metrics

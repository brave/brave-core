/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/process_misc_metrics.h"

#include "brave/browser/misc_metrics/doh_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/misc_metrics/vertical_tab_metrics.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "brave/components/misc_metrics/new_tab_metrics.h"
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
#else
  privacy_hub_metrics_ =
      std::make_unique<misc_metrics::PrivacyHubMetrics>(local_state);
  tab_metrics_ = std::make_unique<misc_metrics::TabMetrics>(local_state);
#endif
  ai_chat_metrics_ = std::make_unique<ai_chat::AIChatMetrics>(local_state);
  doh_metrics_ = std::make_unique<misc_metrics::DohMetrics>(local_state);
  uptime_monitor_ = std::make_unique<misc_metrics::UptimeMonitor>(local_state);
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
#else
PrivacyHubMetrics* ProcessMiscMetrics::privacy_hub_metrics() {
  return privacy_hub_metrics_.get();
}

TabMetrics* ProcessMiscMetrics::tab_metrics() {
  return tab_metrics_.get();
}
#endif

UptimeMonitor* ProcessMiscMetrics::uptime_monitor() {
  return uptime_monitor_.get();
}

ai_chat::AIChatMetrics* ProcessMiscMetrics::ai_chat_metrics() {
  return ai_chat_metrics_.get();
}

void ProcessMiscMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  MenuMetrics::RegisterPrefs(registry);
  NewTabMetrics::RegisterPrefs(registry);
  VerticalTabMetrics::RegisterPrefs(registry);
#else
  PrivacyHubMetrics::RegisterPrefs(registry);
  TabMetrics::RegisterPrefs(registry);
#endif
  ai_chat::AIChatMetrics::RegisterPrefs(registry);
  DohMetrics::RegisterPrefs(registry);
  UptimeMonitor::RegisterPrefs(registry);
}

}  // namespace misc_metrics

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PROCESS_MISC_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_PROCESS_MISC_METRICS_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "build/build_config.h"

class PrefRegistrySimple;
class PrefService;

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class AIChatMetrics;
}  // namespace ai_chat
#endif

namespace misc_metrics {

#if !BUILDFLAG(IS_ANDROID)
class MenuMetrics;
class NewTabMetrics;
class VerticalTabMetrics;
#else
class PrivacyHubMetrics;
class TabMetrics;
#endif
class DohMetrics;
class UptimeMonitor;

class ProcessMiscMetrics {
 public:
  explicit ProcessMiscMetrics(PrefService* local_state);
  ~ProcessMiscMetrics();

  ProcessMiscMetrics(const ProcessMiscMetrics&) = delete;
  ProcessMiscMetrics& operator=(const ProcessMiscMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

#if !BUILDFLAG(IS_ANDROID)
  MenuMetrics* menu_metrics();
  NewTabMetrics* new_tab_metrics();
  VerticalTabMetrics* vertical_tab_metrics();
#else
  PrivacyHubMetrics* privacy_hub_metrics();
  TabMetrics* tab_metrics();
#endif
  UptimeMonitor* uptime_monitor();
#if BUILDFLAG(ENABLE_AI_CHAT)
  ai_chat::AIChatMetrics* ai_chat_metrics();
#endif

 private:
#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<MenuMetrics> menu_metrics_;
  std::unique_ptr<NewTabMetrics> new_tab_metrics_;
  std::unique_ptr<VerticalTabMetrics> vertical_tab_metrics_;
#else
  std::unique_ptr<PrivacyHubMetrics> privacy_hub_metrics_;
  std::unique_ptr<TabMetrics> tab_metrics_;
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<ai_chat::AIChatMetrics> ai_chat_metrics_;
#endif
  std::unique_ptr<DohMetrics> doh_metrics_;
  std::unique_ptr<UptimeMonitor> uptime_monitor_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PROCESS_MISC_METRICS_H_

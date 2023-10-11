/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/process_misc_metrics.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#include "brave/browser/misc_metrics/doh_metrics.h"
#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/misc_metrics/vertical_tab_metrics.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#else
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/ai_chat_metrics.h"
#endif

namespace misc_metrics {

ProcessMiscMetrics::ProcessMiscMetrics(PrefService* local_state) {
#if !BUILDFLAG(IS_ANDROID)
  menu_metrics_ = std::make_unique<MenuMetrics>(local_state);
  vertical_tab_metrics_ = std::make_unique<VerticalTabMetrics>(local_state);
#else
  privacy_hub_metrics_ =
      std::make_unique<misc_metrics::PrivacyHubMetrics>(local_state);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  ai_chat_metrics_ = std::make_unique<ai_chat::AIChatMetrics>(local_state);
#endif
  doh_metrics_ = std::make_unique<misc_metrics::DohMetrics>(local_state);
}

ProcessMiscMetrics::~ProcessMiscMetrics() = default;

#if !BUILDFLAG(IS_ANDROID)
MenuMetrics* ProcessMiscMetrics::menu_metrics() {
  return menu_metrics_.get();
}

VerticalTabMetrics* ProcessMiscMetrics::vertical_tab_metrics() {
  return vertical_tab_metrics_.get();
}
#else
PrivacyHubMetrics* ProcessMiscMetrics::privacy_hub_metrics() {
  return privacy_hub_metrics_.get();
}
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
ai_chat::AIChatMetrics* ProcessMiscMetrics::ai_chat_metrics() {
  return ai_chat_metrics_.get();
}
#endif

void ProcessMiscMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  MenuMetrics::RegisterPrefs(registry);
  VerticalTabMetrics::RegisterPrefs(registry);
#else
  PrivacyHubMetrics::RegisterPrefs(registry);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  ai_chat::AIChatMetrics::RegisterPrefs(registry);
#endif
  DohMetrics::RegisterPrefs(registry);
}

}  // namespace misc_metrics

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace misc_metrics {

ProfileMiscMetricsService::ProfileMiscMetricsService(
    web::BrowserState* browser_state) {
  profile_prefs_ = user_prefs::UserPrefs::Get(browser_state);
  auto* local_state = GetApplicationContext()->GetLocalState();
  if (profile_prefs_ && local_state) {
    ai_chat_metrics_ =
        std::make_unique<ai_chat::AIChatMetrics>(local_state, profile_prefs_);
  }
}

ProfileMiscMetricsService::~ProfileMiscMetricsService() = default;

ai_chat::AIChatMetrics* ProfileMiscMetricsService::GetAIChatMetrics() {
  return ai_chat_metrics_.get();
}

}  // namespace misc_metrics

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_
#define BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace web {
class BrowserState;
}  // namespace web

namespace ai_chat {
class AIChatMetrics;
}  // namespace ai_chat

namespace misc_metrics {

class ProfileMiscMetricsService : public KeyedService {
 public:
  explicit ProfileMiscMetricsService(web::BrowserState* browser_state);
  ~ProfileMiscMetricsService() override;

  ProfileMiscMetricsService(const ProfileMiscMetricsService&) = delete;
  ProfileMiscMetricsService& operator=(const ProfileMiscMetricsService&) =
      delete;

  ai_chat::AIChatMetrics* GetAIChatMetrics();

 private:
  raw_ptr<PrefService> profile_prefs_;

  std::unique_ptr<ai_chat::AIChatMetrics> ai_chat_metrics_;
};

}  // namespace misc_metrics

#endif  // BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

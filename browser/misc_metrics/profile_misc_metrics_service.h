/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_
#define BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"

class SearchEngineTracker;

namespace autofill {
class PersonalDataManager;
}

namespace content {
class BrowserContext;
}  // namespace content

#if !BUILDFLAG(IS_ANDROID)
namespace extensions {
class ExtensionRegistry;
}  // namespace extensions
#endif

namespace history {
class HistoryService;
}  // namespace history

class SearchEngineTracker;

namespace misc_metrics {

#if BUILDFLAG(IS_ANDROID)
class MiscAndroidMetrics;
#else
class ExtensionMetrics;
#endif
class AutofillMetrics;
class LanguageMetrics;
class PageMetrics;

class ProfileMiscMetricsService : public KeyedService {
 public:
  explicit ProfileMiscMetricsService(content::BrowserContext* context);
  ~ProfileMiscMetricsService() override;

  ProfileMiscMetricsService(const ProfileMiscMetricsService&) = delete;
  ProfileMiscMetricsService& operator=(const ProfileMiscMetricsService&) =
      delete;

  void Shutdown() override;

  PageMetrics* GetPageMetrics();
#if BUILDFLAG(IS_ANDROID)
  MiscAndroidMetrics* GetMiscAndroidMetrics();
#endif

 private:
  void OnBraveQuery();

  raw_ptr<SearchEngineTracker> search_engine_tracker_ = nullptr;
  std::unique_ptr<AutofillMetrics> autofill_metrics_ = nullptr;
  std::unique_ptr<LanguageMetrics> language_metrics_ = nullptr;
  std::unique_ptr<PageMetrics> page_metrics_ = nullptr;
#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<MiscAndroidMetrics> misc_android_metrics_ = nullptr;
#else
  std::unique_ptr<ExtensionMetrics> extension_metrics_ = nullptr;
#endif
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

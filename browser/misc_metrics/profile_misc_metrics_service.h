/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_
#define BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

#include <memory>

#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class BrowserContext;
}  // namespace content

namespace misc_metrics {

#if BUILDFLAG(IS_ANDROID)
class MiscAndroidMetrics;
#else
class ExtensionMetrics;
class ThemeMetrics;
#endif
class AutofillMetrics;
class LanguageMetrics;
class PageMetrics;

constexpr char kSearchSuggestEnabledHistogramName[] =
    "Brave.Search.SearchSuggest";

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
  void ReportSimpleMetrics();

  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  std::unique_ptr<AutofillMetrics> autofill_metrics_ = nullptr;
  std::unique_ptr<LanguageMetrics> language_metrics_ = nullptr;
  std::unique_ptr<PageMetrics> page_metrics_ = nullptr;
#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<MiscAndroidMetrics> misc_android_metrics_ = nullptr;
#else
  std::unique_ptr<ExtensionMetrics> extension_metrics_ = nullptr;
  std::unique_ptr<ThemeMetrics> theme_metrics_ = nullptr;
#endif
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_H_

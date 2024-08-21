/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_THEME_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_THEME_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/themes/theme_service_observer.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;
class ThemeService;

namespace misc_metrics {

inline constexpr char kBrowserColorSchemeHistogramName[] =
    "Brave.Theme.BrowserColorScheme";
inline constexpr char kThemeColorDefaultHistogramName[] =
    "Brave.Theme.ThemeColorDefault";

class ThemeMetrics : public ThemeServiceObserver {
 public:
  explicit ThemeMetrics(ThemeService* theme_service);
  ~ThemeMetrics() override;

  ThemeMetrics(const ThemeMetrics&) = delete;
  ThemeMetrics& operator=(const ThemeMetrics&) = delete;

 private:
  void ReportMetrics();

  // ThemeServiceObserver:
  void OnThemeChanged() override;

  raw_ptr<ThemeService> theme_service_;
  PrefChangeRegistrar pref_change_registrar_;
  base::ScopedObservation<ThemeService, ThemeServiceObserver> theme_observer_{
      this};
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_THEME_METRICS_H_

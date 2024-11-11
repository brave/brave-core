/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_VERTICAL_TAB_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_VERTICAL_TAB_METRICS_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sessions/core/session_id.h"

class Browser;
class PrefRegistrySimple;
class PrefService;
class TabStripModel;

namespace misc_metrics {

inline constexpr char kVerticalOpenTabsHistogramName[] =
    "Brave.VerticalTabs.OpenTabs";
inline constexpr char kVerticalGroupTabsHistogramName[] =
    "Brave.VerticalTabs.GroupTabs";
inline constexpr char kVerticalPinnedTabsHistogramName[] =
    "Brave.VerticalTabs.PinnedTabs";

enum class TabCountType {
  kOpen,
  kGroup,
  kPinned,
};

inline constexpr TabCountType kAllTabCountTypes[] = {
    TabCountType::kOpen, TabCountType::kGroup, TabCountType::kPinned};

class VerticalTabBrowserMetrics : public TabStripModelObserver {
 public:
  explicit VerticalTabBrowserMetrics(PrefService* profile_prefs,
                                     base::RepeatingClosure change_callback);
  ~VerticalTabBrowserMetrics() override;

  VerticalTabBrowserMetrics(const VerticalTabBrowserMetrics&) = delete;
  VerticalTabBrowserMetrics& operator=(const VerticalTabBrowserMetrics&) =
      delete;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  size_t GetTabCount(TabCountType count_type) const;

 private:
  void UpdateEnabledStatus();

  bool vertical_tabs_enabled_;
  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;
  base::flat_map<TabCountType, size_t> counts_;
  base::RepeatingClosure change_callback_;
};

class VerticalTabMetrics : public BrowserListObserver {
 public:
  explicit VerticalTabMetrics(PrefService* local_state);
  ~VerticalTabMetrics() override;

  VerticalTabMetrics(const VerticalTabMetrics&) = delete;
  VerticalTabMetrics& operator=(const VerticalTabMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateMetrics();

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;

 private:
  base::flat_map<TabCountType, std::unique_ptr<WeeklyStorage>>
      global_count_storages_;
  base::flat_map<SessionID, std::unique_ptr<VerticalTabBrowserMetrics>>
      browser_metrics_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_VERTICAL_TAB_METRICS_H_

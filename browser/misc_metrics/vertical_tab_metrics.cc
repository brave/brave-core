/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/vertical_tab_metrics.h"

#include <utility>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr int kOpenTabsBuckets[] = {1, 5, 10, 50};
constexpr int kGroupAndPinnedTabsBuckets[] = {2, 5};

const char* GetHistogramNameForCountType(TabCountType count_type) {
  switch (count_type) {
    case TabCountType::kOpen:
      return kVerticalOpenTabsHistogramName;
    case TabCountType::kGroup:
      return kVerticalGroupTabsHistogramName;
    case TabCountType::kPinned:
      return kVerticalPinnedTabsHistogramName;
  }
}

const char* GetStoragePrefNameForCountType(TabCountType count_type) {
  switch (count_type) {
    case TabCountType::kOpen:
      return kMiscMetricsOpenTabsStorage;
    case TabCountType::kGroup:
      return kMiscMetricsGroupTabsStorage;
    case TabCountType::kPinned:
      return kMiscMetricsPinnedTabsStorage;
  }
}

void RecordMaxToHistogramBucket(TabCountType count_type, uint64_t max_value) {
  const char* histogram_name = GetHistogramNameForCountType(count_type);
  switch (count_type) {
    case TabCountType::kOpen:
      p3a_utils::RecordToHistogramBucket(histogram_name, kOpenTabsBuckets,
                                         max_value);
      break;
    case TabCountType::kGroup:
    case TabCountType::kPinned:
      p3a_utils::RecordToHistogramBucket(histogram_name,
                                         kGroupAndPinnedTabsBuckets, max_value);
      break;
  }
}

}  // namespace

VerticalTabBrowserMetrics::VerticalTabBrowserMetrics(
    PrefService* profile_prefs,
    base::RepeatingClosure change_callback)
    : profile_prefs_(profile_prefs), change_callback_(change_callback) {
  for (TabCountType count_type : kAllTabCountTypes) {
    counts_[count_type] = 0;
  }
  pref_change_registrar_.Init(profile_prefs);
  pref_change_registrar_.Add(
      brave_tabs::kVerticalTabsEnabled,
      base::BindRepeating(&VerticalTabBrowserMetrics::UpdateEnabledStatus,
                          base::Unretained(this)));
  UpdateEnabledStatus();
}

VerticalTabBrowserMetrics::~VerticalTabBrowserMetrics() = default;

void VerticalTabBrowserMetrics::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!vertical_tabs_enabled_) {
    return;
  }
  counts_[TabCountType::kOpen] = tab_strip_model->count();
  TabGroupModel* group_model = tab_strip_model->group_model();
  if (group_model != nullptr) {
    counts_[TabCountType::kGroup] = group_model->ListTabGroups().size();
  }
  counts_[TabCountType::kPinned] = tab_strip_model->IndexOfFirstNonPinnedTab();
  change_callback_.Run();
}

size_t VerticalTabBrowserMetrics::GetTabCount(TabCountType count_type) const {
  if (!vertical_tabs_enabled_) {
    return 0;
  }
  return counts_.at(count_type);
}

void VerticalTabBrowserMetrics::UpdateEnabledStatus() {
  vertical_tabs_enabled_ =
      profile_prefs_->GetBoolean(brave_tabs::kVerticalTabsEnabled);
}

VerticalTabMetrics::VerticalTabMetrics(PrefService* local_state) {
  for (TabCountType count_type : kAllTabCountTypes) {
    global_count_storages_[count_type] = std::make_unique<WeeklyStorage>(
        local_state, GetStoragePrefNameForCountType(count_type));
  }

  BrowserList::GetInstance()->AddObserver(this);
}

VerticalTabMetrics::~VerticalTabMetrics() = default;

void VerticalTabMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  for (TabCountType count_type : kAllTabCountTypes) {
    registry->RegisterListPref(GetStoragePrefNameForCountType(count_type));
  }
}

void VerticalTabMetrics::UpdateMetrics() {
  // Add up tab count totals from all windows
  base::flat_map<TabCountType, size_t> current_counts;
  for (const auto& [session_id, browser_metrics] : browser_metrics_) {
    for (TabCountType count_type : kAllTabCountTypes) {
      current_counts[count_type] += browser_metrics->GetTabCount(count_type);
    }
  }
  // Report histograms for each tab count type, if the
  // particular count type is non-zero.
  for (TabCountType count_type : kAllTabCountTypes) {
    WeeklyStorage* storage = global_count_storages_[count_type].get();
    storage->ReplaceTodaysValueIfGreater(current_counts[count_type]);
    uint64_t max_value = storage->GetHighestValueInWeek();
    if (max_value > 0) {
      RecordMaxToHistogramBucket(count_type, max_value);
    }
  }
}

void VerticalTabMetrics::OnBrowserAdded(Browser* browser) {
  if (!browser->is_type_normal()) {
    return;
  }
  Profile* profile = browser->profile();
  if (!profile || profile->IsOffTheRecord() || !profile->IsRegularProfile()) {
    // Do not monitor incognito windows.
    return;
  }
  PrefService* profile_prefs = profile->GetPrefs();
  CHECK(profile_prefs);
  SessionID session_id = browser->session_id();
  browser_metrics_[session_id] = std::make_unique<VerticalTabBrowserMetrics>(
      profile_prefs, base::BindRepeating(&VerticalTabMetrics::UpdateMetrics,
                                         base::Unretained(this)));
  browser->tab_strip_model()->AddObserver(browser_metrics_[session_id].get());
}

void VerticalTabMetrics::OnBrowserRemoved(Browser* browser) {
  browser_metrics_.erase(browser->session_id());
}

}  // namespace misc_metrics

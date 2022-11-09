/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/menu_metrics.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace misc_metrics {

namespace {

const MenuGroup kAllMenuGroups[] = {
    MenuGroup::kTabWindow, MenuGroup::kBraveFeatures, MenuGroup::kBrowserViews};

const char kTabWindowPrefKey[] = "tab_window";
const char kBraveFeaturesPrefKey[] = "brave_features";
const char kBrowserViewsPrefKey[] = "browser_views";

const char* GetMenuGroupPrefKey(MenuGroup group) {
  switch (group) {
    case MenuGroup::kTabWindow:
      return kTabWindowPrefKey;
    case MenuGroup::kBraveFeatures:
      return kBraveFeaturesPrefKey;
    case MenuGroup::kBrowserViews:
      return kBrowserViewsPrefKey;
  }
  NOTREACHED();
}

}  // namespace

const char kFrequentMenuGroupHistogramName[] =
    "Brave.Toolbar.FrequentMenuGroup";

MenuMetrics::MenuMetrics(PrefService* local_state)
    : local_state_(local_state) {}

MenuMetrics::~MenuMetrics() = default;

void MenuMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsMenuGroupActionCounts);
}

void MenuMetrics::RecordMenuGroupAction(MenuGroup group) {
  const char* group_pref_key = GetMenuGroupPrefKey(group);

  VLOG(2) << "MenuMetrics: recorded " << group_pref_key;

  DictionaryPrefUpdate update(local_state_, kMiscMetricsMenuGroupActionCounts);
  base::Value::Dict& update_dict = update->GetDict();
  for (MenuGroup it : kAllMenuGroups) {
    if (!menu_group_access_counts_.contains(it)) {
      double value =
          update_dict.FindDouble(GetMenuGroupPrefKey(it)).value_or(0);
      menu_group_access_counts_[it] = value;
    }
  }
  menu_group_access_counts_[group]++;
  update_dict.Set(GetMenuGroupPrefKey(group), menu_group_access_counts_[group]);
  const auto& result = base::ranges::max_element(
      menu_group_access_counts_.begin(), menu_group_access_counts_.end(),
      [](const auto& a, const auto& b) { return a.second < b.second; });
  if (result == menu_group_access_counts_.end()) {
    return;
  }
  int histogram_value = -1;
  switch (result->first) {
    case MenuGroup::kTabWindow:
      histogram_value = 0;
      break;
    case MenuGroup::kBraveFeatures:
      histogram_value = 1;
      break;
    case MenuGroup::kBrowserViews:
      histogram_value = 2;
      break;
    default:
      NOTREACHED();
      return;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kFrequentMenuGroupHistogramName, histogram_value,
                             3);
}

}  // namespace misc_metrics

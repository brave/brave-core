/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

  update_dict.Set(group_pref_key,
                  update_dict.FindDouble(group_pref_key).value_or(0) + 1);

  int histogram_value;
  if (current_max_group_.has_value() && current_max_group_->first == group) {
    // No need to check for max element if we added to the last known max group.
    histogram_value = current_max_group_->second;
  } else {
    const auto& result = base::ranges::max_element(
        update_dict.cbegin(), update_dict.cend(),
        [](const auto& a, const auto& b) {
          return a.second.GetDouble() < b.second.GetDouble();
        });
    if (result == update_dict.cend()) {
      return;
    }
    if (result->first == kTabWindowPrefKey) {
      histogram_value = 0;
      current_max_group_ = std::make_pair(MenuGroup::kTabWindow, 0);
    } else if (result->first == kBraveFeaturesPrefKey) {
      histogram_value = 1;
      current_max_group_ = std::make_pair(MenuGroup::kBraveFeatures, 1);
    } else if (result->first == kBrowserViewsPrefKey) {
      histogram_value = 2;
      current_max_group_ = std::make_pair(MenuGroup::kBrowserViews, 2);
    } else {
      return;
    }
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kFrequentMenuGroupHistogramName, histogram_value,
                             3);
}

}  // namespace misc_metrics

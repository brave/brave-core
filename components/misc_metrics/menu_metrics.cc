/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/menu_metrics.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace misc_metrics {

namespace {

constexpr char kTabWindowPrefKey[] = "tab_window";
constexpr char kBraveFeaturesPrefKey[] = "brave_features";
constexpr char kBrowserViewsPrefKey[] = "browser_views";

constexpr base::TimeDelta kUpdateInterval = base::Days(1);

constexpr int kMenuOpenBuckets[] = {0, 5, 15, 29, 49};

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

MenuMetrics::MenuMetrics(PrefService* local_state)
    : local_state_(local_state),
      menu_shown_storage_(local_state, kMiscMetricsMenuShownStorage),
      menu_dismiss_storage_(local_state, kMiscMetricsMenuDismissStorage) {
  Update();
}

MenuMetrics::~MenuMetrics() = default;

void MenuMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsMenuGroupActionCounts);
  registry->RegisterListPref(kMiscMetricsMenuShownStorage);
  registry->RegisterListPref(kMiscMetricsMenuDismissStorage);
}

void MenuMetrics::RecordMenuGroupAction(MenuGroup group) {
  const char* group_pref_key = GetMenuGroupPrefKey(group);

  VLOG(2) << "MenuMetrics: recorded " << group_pref_key;

  ScopedDictPrefUpdate update(local_state_, kMiscMetricsMenuGroupActionCounts);
  base::Value::Dict& update_dict = update.Get();

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

void MenuMetrics::RecordMenuShown() {
  VLOG(2) << "MenuMetrics: menu shown";
  menu_shown_storage_.AddDelta(1);
  RecordMenuDismissRate();
  RecordMenuOpens();
}

void MenuMetrics::RecordMenuDismiss() {
  VLOG(2) << "MenuMetrics: menu dismiss";
  menu_dismiss_storage_.AddDelta(1);
  RecordMenuDismissRate();
}

void MenuMetrics::RecordMenuDismissRate() {
  const double shown_sum = menu_shown_storage_.GetWeeklySum();
  const double dismiss_sum = menu_dismiss_storage_.GetWeeklySum();

  int answer = 0;
  if (shown_sum != 0) {
    const double rate = dismiss_sum / shown_sum;

    VLOG(2) << "MenuMetrics: menu dismiss rate: " << rate;

    if (rate < 0.25) {
      answer = 1;
    } else if (rate >= 0.25 && rate < 0.5) {
      answer = 2;
    } else if (rate >= 0.5 && rate < 0.75) {
      answer = 3;
    } else if (rate >= 0.75) {
      answer = 4;
    }
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kMenuDismissRateHistogramName, answer, 5);
}

void MenuMetrics::RecordMenuOpens() {
  p3a_utils::RecordToHistogramBucket(kMenuOpensHistogramName, kMenuOpenBuckets,
                                     menu_shown_storage_.GetWeeklySum());
}

void MenuMetrics::Update() {
  RecordMenuDismissRate();
  RecordMenuOpens();
  update_timer_.Start(FROM_HERE, base::Time::Now() + kUpdateInterval, this,
                      &MenuMetrics::Update);
}

}  // namespace misc_metrics

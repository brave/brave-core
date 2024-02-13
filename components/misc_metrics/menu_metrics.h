/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_

#include <optional>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

enum class MenuGroup {
  kTabWindow,
  kBraveFeatures,
  kBrowserViews,
};

inline constexpr char kFrequentMenuGroupHistogramName[] =
    "Brave.Toolbar.FrequentMenuGroup";
inline constexpr char kMenuDismissRateHistogramName[] =
    "Brave.Toolbar.MenuDismissRate";
inline constexpr char kMenuOpensHistogramName[] = "Brave.Toolbar.MenuOpens";

class MenuMetrics {
 public:
  explicit MenuMetrics(PrefService* local_state);
  ~MenuMetrics();

  MenuMetrics(const MenuMetrics&) = delete;
  MenuMetrics& operator=(const MenuMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Records a usage of the relevant menu group, and records an answer
  // for the following P3A question:
  // What menu functionality group do you use the most?
  // 0. Tab & window actions (new tab/new window)
  // 1. Brave features (Wallet, Rewards, Sync)
  // 2. Browser views (History, Bookmarks, Extensions, Settings)
  void RecordMenuGroupAction(MenuGroup group);

  // Increments weekly count of menu appearances in order to calculate the
  // menu dismiss rate P3A question.
  void RecordMenuShown();
  // Increments weekly count of menu dismisses, and records an answer
  // for the following P3A question:
  // How often is the menu triggered and dismissed without an action taken in
  // the past week? 0. Menu was not opened in the past week
  // 1. Less than 25% (exclusive) of opens
  // 2. Between 25% (inclusive) and 50% (exclusive) of opens
  // 3. Between 50% (inclusive) and 75% (exclusive) of opens
  // 4. More than 75% of opens
  void RecordMenuDismiss();

 private:
  void RecordMenuDismissRate();
  void RecordMenuOpens();

  void Update();

  std::optional<std::pair<MenuGroup, int>> current_max_group_;

  raw_ptr<PrefService> local_state_ = nullptr;
  WeeklyStorage menu_shown_storage_;
  WeeklyStorage menu_dismiss_storage_;

  base::WallClockTimer update_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_LIST_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_LIST_P3A_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace brave_shields {

class FilterListCatalogEntry;

inline constexpr char kFilterListUsageHistogramName[] =
    "Brave.Shields.FilterLists";
inline constexpr char kAdBlockOnlyModeEnabledHistogramName[] =
    "Brave.Shields.AdBlockOnlyModeEnabled";

class AdBlockListP3A {
 public:
  explicit AdBlockListP3A(PrefService* local_state);
  ~AdBlockListP3A();

  AdBlockListP3A(const AdBlockListP3A&) = delete;
  AdBlockListP3A& operator=(const AdBlockListP3A&) = delete;

  void ReportFilterListUsage();

  void OnFilterListCatalogLoaded(
      const std::vector<FilterListCatalogEntry>& entries);

 private:
  void ReportAdBlockOnlyEnabled();

  raw_ptr<PrefService> local_state_;
  base::flat_set<std::string> default_filter_list_uuids_;
  base::WallClockTimer adblock_only_mode_report_timer_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_LIST_P3A_H_

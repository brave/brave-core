/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/browser/sidebar_p3a.h"

#include "base/check_is_test.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "components/prefs/pref_service.h"

namespace sidebar {
namespace p3a {

SidebarP3A::SidebarP3A(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {
  profile_pref_change_registrar_.Init(profile_prefs);
  profile_pref_change_registrar_.Add(
      kSidebarShowOption, base::BindRepeating(&SidebarP3A::RecordEnabledSetting,
                                              base::Unretained(this), true));

  RecordEnabledSetting(false);
}

SidebarP3A::~SidebarP3A() = default;

void SidebarP3A::RecordEnabledSetting(bool setting_changed) {
  // Some non-sidebar unittest could not register the prefs.
  if (!profile_prefs_->FindPreference(kSidebarShowOption)) {
    CHECK_IS_TEST();
    return;
  }

  auto setting = static_cast<SidebarService::ShowSidebarOption>(
      profile_prefs_->GetInteger(kSidebarShowOption));
  bool is_enabled = setting != SidebarService::ShowSidebarOption::kShowNever;
  int answer = is_enabled ? 1 : (INT_MAX - 1);
  UMA_HISTOGRAM_EXACT_LINEAR(kSidebarEnabledHistogramName, answer, 2);

  auto sidebar_default_mode = features::GetSidebarDefaultMode();
  const char* setting_change_histogram_name;
  if (sidebar_default_mode == features::SidebarDefaultMode::kAlwaysOn) {
    setting_change_histogram_name = kSettingChangeSidebarEnabledAHistogramName;
  } else if (sidebar_default_mode == features::SidebarDefaultMode::kOnOneShot) {
    setting_change_histogram_name = kSettingChangeSidebarEnabledBHistogramName;
  } else {
    UMA_HISTOGRAM_EXACT_LINEAR(kSettingChangeSidebarEnabledAHistogramName,
                               INT_MAX - 1, 3);
    UMA_HISTOGRAM_EXACT_LINEAR(kSettingChangeSidebarEnabledBHistogramName,
                               INT_MAX - 1, 3);
    return;
  }

  int setting_change_answer = 0;

  if (!profile_prefs_->GetBoolean(kSidebarSettingChangeInitialP3AReport)) {
    profile_prefs_->SetBoolean(kSidebarSettingChangeInitialP3AReport, true);
  } else if (setting_changed) {
    if (setting == SidebarService::ShowSidebarOption::kShowOnMouseOver) {
      setting_change_answer = 1;
    } else if (setting == SidebarService::ShowSidebarOption::kShowNever) {
      setting_change_answer = 2;
    } else {
      return;
    }
  } else {
    return;
  }

  base::UmaHistogramExactLinear(setting_change_histogram_name,
                                setting_change_answer, 3);
}

}  // namespace p3a
}  // namespace sidebar

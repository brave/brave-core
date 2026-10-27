/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/bookmark/bookmark_helper.h"

#include "base/feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/menus/simple_menu_model.h"

namespace brave {

namespace {

BookmarkBarState GetBookmarkBarState(PrefService* prefs) {
  // kShowBookmarkBar has higher priority and the bookmark bar is shown always.
  if (prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar)) {
    return BookmarkBarState::kAlways;
  }
  // kShowBookmarkBar is false, kAlwaysShowBookmarkBarOnNTP is true
  // -> the bookmark bar is shown only for NTP.
  if (prefs->GetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP)) {
    return BookmarkBarState::kNtp;
  }
  // NEVER show the bookmark bar.
  return BookmarkBarState::kNever;
}

}  // namespace

bool IsFloatingBookmarkBarEnabled(const PrefService& prefs) {
  return base::FeatureList::IsEnabled(features::kBraveFloatingBookmarkBar) &&
         prefs.GetBoolean(kFloatingBookmarkBar);
}

void AddFloatingBookmarkBarMenuItem(ui::SimpleMenuModel& menu) {
  if (!base::FeatureList::IsEnabled(features::kBraveFloatingBookmarkBar)) {
    return;
  }
  const auto index = menu.GetIndexOfCommandId(IDC_BOOKMARK_BAR_SUBMENU);
  if (!index) {
    return;
  }
  // Both native bookmark visibility submenus are SimpleMenuModels.
  auto* submenu =
      static_cast<ui::SimpleMenuModel*>(menu.GetSubmenuModelAt(*index));
  const auto last_mode =
      submenu->GetIndexOfCommandId(IDC_BOOKMARK_BAR_SUBMENU_ONLY_ON_NTP);
  if (last_mode) {
    submenu->InsertCheckItemWithStringIdAt(
        *last_mode + 1, IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING,
        IDS_SETTINGS_APPEARANCE_SETTINGS_FLOATING_BOOKMARKS_BAR);
  }
}

bool CanSetFloatingBookmarkBar(const PrefService& prefs) {
  return base::FeatureList::IsEnabled(features::kBraveFloatingBookmarkBar) &&
         !prefs.IsManagedPreference(kFloatingBookmarkBar) &&
         !prefs.IsManagedPreference(
             bookmarks::prefs::kBookmarkBarVisibilityState) &&
         !prefs.IsManagedPreference(bookmarks::prefs::kShowBookmarkBar);
}

std::optional<bool> IsFloatingBookmarkBarCommandChecked(
    int command_id,
    const PrefService& prefs) {
  if (command_id == IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING) {
    return IsFloatingBookmarkBarEnabled(prefs);
  }
  if (IsFloatingBookmarkBarEnabled(prefs) &&
      (command_id == IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_SHOW ||
       command_id == IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_HIDE ||
       command_id == IDC_BOOKMARK_BAR_SUBMENU_ONLY_ON_NTP)) {
    return false;
  }
  return std::nullopt;
}

bool HandleFloatingBookmarkBarCommand(int command_id, PrefService& prefs) {
  const bool floating =
      command_id == IDC_BOOKMARK_BAR_SUBMENU_AUTO_HIDE_FLOATING;
  if (!floating && command_id != IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_SHOW &&
      command_id != IDC_BOOKMARK_BAR_SUBMENU_ALWAYS_HIDE &&
      command_id != IDC_BOOKMARK_BAR_SUBMENU_ONLY_ON_NTP) {
    return false;
  }
  if (!base::FeatureList::IsEnabled(features::kBraveFloatingBookmarkBar)) {
    return floating;
  }
  if (!CanSetFloatingBookmarkBar(prefs)) {
    return true;
  }
  prefs.SetBoolean(kFloatingBookmarkBar, floating);
  return floating;
}

void SetBookmarkStateForTesting(BookmarkBarState state, PrefService* prefs) {
  if (state == BookmarkBarState::kAlways) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, false);
  } else if (state == BookmarkBarState::kNtp) {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, true);
  } else {
    prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
    prefs->SetBoolean(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP, false);
  }
}

// Migrate to use upstream's kBookmarkBarVisibilityState.
void MigrateBookmarkState(PrefService* prefs) {
  // Nothing to migrate. Clean kShowBookmarkBar means
  // kAlwaysShowBookmarkBarOnNTP is also default state.
  if (auto* pref =
          prefs->FindPreference(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP);
      pref && pref->IsDefaultValue()) {
    return;
  }

  bookmarks::BookmarkBarVisibilityState state =
      bookmarks::BookmarkBarVisibilityState::kAlwaysHide;
  const BookmarkBarState deprecated_state = GetBookmarkBarState(prefs);
  if (deprecated_state == BookmarkBarState::kAlways) {
    state = bookmarks::BookmarkBarVisibilityState::kAlwaysShow;
  } else if (deprecated_state == BookmarkBarState::kNtp) {
    state = bookmarks::BookmarkBarVisibilityState::kOnlyShowOnNtp;
  }
  prefs->SetInteger(bookmarks::prefs::kBookmarkBarVisibilityState,
                    static_cast<int>(state));

  // Didn't clear kShowBookmarkBar as it's upstream prefs and it's still
  // synced with kBookmarkBarVisibilityState.
  prefs->ClearPref(bookmarks::prefs::kAlwaysShowBookmarkBarOnNTP);
}

}  // namespace brave

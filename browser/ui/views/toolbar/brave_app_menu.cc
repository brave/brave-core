/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_app_menu.h"

#include <memory>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/misc_metrics/menu_metrics.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/menu/menu_item_view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_status_label.h"
#include "brave/browser/ui/views/toolbar/brave_vpn_toggle_button.h"
#endif

using views::MenuItemView;

namespace {

// Copied from app_menu.cc to dump w/o crashing
bool IsBookmarkCommand(int command_id) {
  return command_id >= IDC_FIRST_UNBOUNDED_MENU &&
         ((command_id - IDC_FIRST_UNBOUNDED_MENU) %
              AppMenuModel::kNumUnboundedMenuTypes ==
          0);
}

}  // namespace

BraveAppMenu::BraveAppMenu(Browser* browser,
                           ui::MenuModel* model,
                           int run_types)
    : AppMenu(browser, model, run_types),
      menu_metrics_(
          g_brave_browser_process->process_misc_metrics()->menu_metrics()) {
  DCHECK(menu_metrics_);
  UpdateMenuItemView();
}

BraveAppMenu::~BraveAppMenu() = default;

void BraveAppMenu::RunMenu(views::MenuButtonController* host) {
  AppMenu::RunMenu(host);
  menu_metrics_->RecordMenuShown();
}

void BraveAppMenu::ExecuteCommand(int command_id, int mouse_event_flags) {
  // Suspect that the entry is null but can't imagine which command causes it.
  // See
  // https://github.com/brave/brave-browser/issues/37862#issuecomment-2078553575
  if (!IsBookmarkCommand(command_id) && command_id != IDC_EDIT_MENU &&
      command_id != IDC_ZOOM_MENU &&
      command_id_to_entry_.find(command_id) == command_id_to_entry_.end()) {
    LOG(ERROR) << __func__ << " entry should be exsit for " << command_id;
    SCOPED_CRASH_KEY_NUMBER("BraveAppMenu", "command_id", command_id);
    base::debug::DumpWithoutCrashing();
    return;
  }

  AppMenu::ExecuteCommand(command_id, mouse_event_flags);
  RecordMenuUsage(command_id);
}

void BraveAppMenu::OnMenuClosed(views::MenuItemView* menu) {
  AppMenu::OnMenuClosed(menu);
  if (menu == nullptr) {
    menu_metrics_->RecordMenuDismiss();
  }
}

void BraveAppMenu::RecordMenuUsage(int command_id) {
  misc_metrics::MenuGroup group;

  switch (command_id) {
    case IDC_NEW_WINDOW:
    case IDC_NEW_TAB:
    case IDC_NEW_INCOGNITO_WINDOW:
    case IDC_NEW_OFFTHERECORD_WINDOW_TOR:
    case IDC_OPEN_GUEST_PROFILE:
      group = misc_metrics::MenuGroup::kTabWindow;
      break;
    case IDC_SHOW_BRAVE_WALLET:
    case IDC_SHOW_BRAVE_SYNC:
    case IDC_SHOW_BRAVE_REWARDS:
      group = misc_metrics::MenuGroup::kBraveFeatures;
      break;
    case IDC_SHOW_HISTORY:
    case IDC_MANAGE_EXTENSIONS:
    case IDC_SHOW_BOOKMARK_MANAGER:
    case IDC_BOOKMARK_THIS_TAB:
    case IDC_BOOKMARK_ALL_TABS:
    case IDC_SHOW_BOOKMARK_BAR:
    case IDC_IMPORT_SETTINGS:
    case IDC_OPTIONS:
    case IDC_SHOW_DOWNLOADS:
      group = misc_metrics::MenuGroup::kBrowserViews;
      break;
    default:
      if (command_id >= IDC_FIRST_UNBOUNDED_MENU) {
        group = misc_metrics::MenuGroup::kBrowserViews;
      } else {
        return;
      }
  }
  menu_metrics_->RecordMenuGroupAction(group);
}

void BraveAppMenu::UpdateMenuItemView() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  CHECK(root_menu_item());
  if (auto* menu_item =
          root_menu_item()->GetMenuItemByID(IDC_TOGGLE_BRAVE_VPN)) {
    menu_item->AddChildView(std::make_unique<BraveVPNStatusLabel>(browser_));
    menu_item->AddChildView(std::make_unique<BraveVPNToggleButton>(browser_));
  }
#endif
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_browser_tab_menu_model_delegate.h"

#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"

namespace brave {

BraveBrowserTabMenuModelDelegate::BraveBrowserTabMenuModelDelegate(
    SessionID session_id,
    const Profile* profile,
    const web_app::AppBrowserController* app_controller,
    tab_groups::TabGroupSyncService* tgss,
    VerticalTabController* vertical_tab_controller)
    : chrome::BrowserTabMenuModelDelegate(session_id,
                                          profile,
                                          app_controller,
                                          tgss),
      vertical_tab_controller_(vertical_tab_controller) {}

BraveBrowserTabMenuModelDelegate::~BraveBrowserTabMenuModelDelegate() = default;

bool BraveBrowserTabMenuModelDelegate::ShouldShowBraveVerticalTab() {
  return vertical_tab_controller_->ShouldShowBraveVerticalTabs();
}

}  // namespace brave

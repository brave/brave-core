/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_BROWSER_TAB_MENU_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_BROWSER_TAB_MENU_MODEL_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/browser_tab_menu_model_delegate.h"

class BrowserWindowInterface;
class Profile;

namespace brave {

// Implementation of TabMenuModelDelegate which extends
// BrowserTabMenuModelDelegate to provide Brave-specific functionality for tab
// context menus.
class BraveBrowserTabMenuModelDelegate
    : public chrome::BrowserTabMenuModelDelegate {
 public:
  BraveBrowserTabMenuModelDelegate(
      SessionID session_id,
      const Profile* profile,
      const web_app::AppBrowserController* app_controller,
      tab_groups::TabGroupSyncService* tgss,
      BrowserWindowInterface* browser_window);
  ~BraveBrowserTabMenuModelDelegate() override;

  BraveBrowserTabMenuModelDelegate(const BraveBrowserTabMenuModelDelegate&) =
      delete;
  BraveBrowserTabMenuModelDelegate& operator=(
      const BraveBrowserTabMenuModelDelegate&) = delete;

  // TabMenuModelDelegate overrides for Brave-specific functionality:
  bool ShouldShowBraveVerticalTab() override;

 private:
  raw_ptr<BrowserWindowInterface> browser_window_ = nullptr;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_BROWSER_TAB_MENU_MODEL_DELEGATE_H_

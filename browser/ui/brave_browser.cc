/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"

#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/new_tab_page_third_party/new_tab_page_third_party_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"

BraveBrowser::BraveBrowser(BrowserWindowCreateParams params)
    : Browser(std::move(params)) {
  auto* brave_browser_window = BraveBrowserWindow::FromBrowser(this);
  if (!brave_browser_window) {
    // The window is not a BrowserView, e.g. a WebUIBrowserWindow.
    return;
  }

  if (auto* sidebar_controller = sidebar::SidebarController::From(this)) {
    // TODO(https://github.com/brave/brave-browser/issues/45633): Cleanup this.
    // Below call order is important.
    // When reaches here, Sidebar UI is setup in BraveBrowserView but
    // not initialized. It's just empty because sidebar controller/model is not
    // ready yet. BraveBrowserView is instantiated by the ctor of Browser.
    // So, initializing sidebar controller/model here and then ask to initialize
    // sidebar UI. After that, UI will be updated for model's change.
    sidebar_controller->SetSidebar(brave_browser_window->InitSidebar());
  }

  // As browser window(BrowserView) is initialized before fullscreen controller
  // is ready, it's difficult to know when browsr window can listen.
  // Notify exact timing to do it.
  CHECK(ExclusiveAccessManager::From(this));
  brave_browser_window->ReadyToListenFullscreenChanges();
}

BraveBrowser::~BraveBrowser() = default;

void BraveBrowser::TabStripEmpty() {
  if (GetProfile()->GetPrefs()->GetBoolean(kEnableClosingLastTab) ||
      GetType() != BrowserWindowInterface::Type::TYPE_NORMAL ||
      ignore_enable_closing_last_tab_pref_) {
    Browser::TabStripEmpty();
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<BraveBrowser> browser) {
                       if (browser) {
                         chrome::AddTabAt(browser.get(),
                                          chrome::GetNewTabURL(browser.get()),
                                          /*index=*/-1, /*foreground=*/true,
                                          /*group=*/std::nullopt,
                                          /*pinned=*/false);
                       }
                     },
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveBrowser::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  Browser::OnTabStripModelChanged(tab_strip_model, change, selection);

  if (!GetProfile()->GetPrefs()->GetBoolean(kEnableClosingLastTab) &&
      change.type() == TabStripModelChange::kRemoved) {
    for (const auto& contents : change.GetRemove()->contents) {
      // If there is no tab after this change for inserting them to
      // another window, this browser should be closed.
      if (contents.remove_reason ==
              TabRemovedReason::kInsertedIntoOtherTabStrip &&
          tab_strip_model->empty()) {
        // Each removed can only have same reason. so safe to early return here.
        ignore_enable_closing_last_tab_pref_ = true;
        break;
      }
    }
  }

  // Check if all tabs we set to ignore onbeforeunload handler are closed.
  if (!tabs_closing_with_onbeforeunload_ignore_.empty() &&
      change.type() == TabStripModelChange::Type::kRemoved) {
    for (auto& removed_tab : change.GetRemove()->contents) {
      tabs_closing_with_onbeforeunload_ignore_.erase(
          removed_tab.tab->GetHandle());
    }
  }
}

void BraveBrowser::SetTabsToIgnoreBeforeUnloadHandlers(
    const base::flat_set<tabs::TabHandle>& for_contents) {
  tabs_closing_with_onbeforeunload_ignore_ = for_contents;
}

bool BraveBrowser::ShouldIgnoreBeforeUnloadHandlerForTab(
    tabs::TabHandle handle) const {
  return tabs_closing_with_onbeforeunload_ignore_.contains(handle);
}

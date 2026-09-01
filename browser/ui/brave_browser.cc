/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"

#include <optional>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/lifetime/browser_close_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/unload_controller.h"
#include "chrome/browser/ui/webui/new_tab_page/new_tab_page_ui.h"
#include "chrome/browser/ui/webui/new_tab_page_third_party/new_tab_page_third_party_ui.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/browser/ui/webui_browser/webui_browser.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"

namespace {

bool g_suppress_dialog_for_testing = false;

}  // namespace

// static
void BraveBrowser::SuppressBrowserWindowClosingDialogForTesting(bool suppress) {
  g_suppress_dialog_for_testing = suppress;
}

BraveBrowser::BraveBrowser(BrowserWindowCreateParams params)
    : Browser(std::move(params)) {
  if (auto* sidebar_controller = GetFeatures().sidebar_controller()) {
    // TODO(https://github.com/brave/brave-browser/issues/45633): Cleanup this.
    // Below call order is important.
    // When reaches here, Sidebar UI is setup in BraveBrowserView but
    // not initialized. It's just empty because sidebar controller/model is not
    // ready yet. BraveBrowserView is instantiated by the ctor of Browser.
    // So, initializing sidebar controller/model here and then ask to initialize
    // sidebar UI. After that, UI will be updated for model's change.
    sidebar_controller->SetSidebar(
        BraveBrowserWindow::FromBrowser(this)->InitSidebar());
  }

  if (webui_browser::IsWebUIBrowserEnabled() &&
      GetType() == BrowserWindowInterface::Type::TYPE_NORMAL) {
    // WebUIBrowserWindow was created in Browser's c'tor (in
    // BrowserWindow::CreateBrowserWindow), not a BraveBrowserWindow.
    return;
  }

  // As browser window(BrowserView) is initialized before fullscreen controller
  // is ready, it's difficult to know when browsr window can listen.
  // Notify exact timing to do it.
  CHECK(GetFeatures().exclusive_access_manager());
  BraveBrowserWindow::FromBrowser(this)->ReadyToListenFullscreenChanges();
}

BraveBrowser::~BraveBrowser() = default;

void BraveBrowser::OnTabClosing(tabs::TabInterface* tab,
                                bool* had_active_modal_dialog) {
  Browser::OnTabClosing(tab, had_active_modal_dialog);

  if (!AreAllTabsSharedPinnedTabs()) {
    return;
  }

  bool more_than_one = false;
  ProfileBrowserCollection::GetForProfile(GetProfile())
      ->ForEach([this, &more_than_one](BrowserWindowInterface* browser) {
        if (!more_than_one) {
          more_than_one = true;
          return true;
        }

        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, base::BindOnce(
                           [](base::WeakPtr<BraveBrowser> browser) {
                             if (browser) {
                               // We don't want close confirm dialog to show up.
                               // In this case, Shared pinned tabs will be moved
                               // to another window, so we don't have to warn
                               // users.
                               UnloadController::From(browser.get())
                                   ->set_confirmed_to_close(true);
                               chrome::CloseWindow(browser.get());
                             }
                           },
                           weak_ptr_factory_.GetWeakPtr()));

        return false;
      });
}

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

  // sidebar() can return a nullptr in unit tests.
  auto* sidebar_controller = GetFeatures().sidebar_controller();
  if (!sidebar_controller || !sidebar_controller->sidebar()) {
    return;
  }
  // We need to update sidebar UI whenever active tab is changed or
  // inactive tab is added/removed.
  if (change.type() == TabStripModelChange::Type::kInserted ||
      change.type() == TabStripModelChange::Type::kRemoved ||
      selection.active_tab_changed()) {
    sidebar_controller->sidebar()->UpdateSidebarItemsState();
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

bool BraveBrowser::ShouldAskForBrowserClosingBeforeHandlers() {
  if (g_suppress_dialog_for_testing) {
    return false;
  }

  // Don't need to ask when application closing is in-progress.
  if (BrowserCloseManager::BrowserClosingStarted()) {
    return false;
  }

  if (UnloadController::From(this)->confirmed_to_close()) {
    return false;
  }

  PrefService* prefs = GetProfile()->GetPrefs();
  if (!prefs->GetBoolean(kEnableWindowClosingConfirm)) {
    return false;
  }

  // Only launch confirm dialog while closing when browser has multiple tabs.
  return tab_strip_model()->count() > 1;
}

bool BraveBrowser::AreAllTabsSharedPinnedTabs() {
  if (!base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    return false;
  }

  if (GetType() != BrowserWindowInterface::Type::TYPE_NORMAL) {
    return false;
  }

  if (!GetProfile()->GetPrefs()->GetBoolean(brave_tabs::kSharedPinnedTab)) {
    return false;
  }

  return tab_strip_model()->count() > 0 &&
         tab_strip_model()->count() ==
             tab_strip_model()->IndexOfFirstNonPinnedTab();
}

void BraveBrowser::SetTabsToIgnoreBeforeUnloadHandlers(
    const base::flat_set<tabs::TabHandle>& for_contents) {
  tabs_closing_with_onbeforeunload_ignore_ = for_contents;
}

bool BraveBrowser::ShouldIgnoreBeforeUnloadHandlerForTab(
    tabs::TabHandle handle) const {
  return tabs_closing_with_onbeforeunload_ignore_.contains(handle);
}

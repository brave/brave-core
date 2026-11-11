/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/closing_last_tab_controller.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/prefs/pref_service.h"

namespace {

bool WasTabMovedToOtherWindow(const TabStripModelChange& change) {
  // Every tab removed by one change shares a reason, so the first is enough.
  const auto& removed = change.GetRemove()->contents;
  return !removed.empty() && removed.front().remove_reason ==
                                 TabRemovedReason::kInsertedIntoOtherTabStrip;
}

}  // namespace

DEFINE_USER_DATA(ClosingLastTabController);

ClosingLastTabController::ClosingLastTabController(
    BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  browser.GetTabStripModel()->AddObserver(this);
}

ClosingLastTabController::~ClosingLastTabController() = default;

// static
ClosingLastTabController* ClosingLastTabController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

bool ClosingLastTabController::MaybeKeepWindowOpen() {
  if (ShouldCloseWindowWhenTabStripEmpty()) {
    return false;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ClosingLastTabController::AddNewTabForEmptyTabStrip,
                     weak_ptr_factory_.GetWeakPtr()));
  return true;
}

void ClosingLastTabController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (change.type() != TabStripModelChange::kRemoved) {
    return;
  }

  // A window whose tabs all went to another window has nothing to replace, so
  // it closes despite the pref.
  if (!browser_->GetProfile()->GetPrefs()->GetBoolean(
          brave_tabs::kEnableClosingLastTab) &&
      tab_strip_model->empty() && WasTabMovedToOtherWindow(change)) {
    ignore_enable_closing_last_tab_pref_ = true;
  }

  // TabStripEmpty() never fires for a strip holding only the panel's tab.
  if (sidebar::IsWebPanelOnlyTab(base::to_address(browser_))) {
    // Posted: TabStripModel forbids mutating it from within a notification.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&ClosingLastTabController::MaybeHandleOnlyWebPanelLeft,
                       weak_ptr_factory_.GetWeakPtr(),
                       WasTabMovedToOtherWindow(change)));
  }
}

bool ClosingLastTabController::ShouldCloseWindowWhenTabStripEmpty() const {
  return browser_->GetProfile()->GetPrefs()->GetBoolean(
             brave_tabs::kEnableClosingLastTab) ||
         browser_->GetType() != BrowserWindowInterface::Type::TYPE_NORMAL ||
         ignore_enable_closing_last_tab_pref_;
}

void ClosingLastTabController::AddNewTabForEmptyTabStrip() {
  chrome::AddTabAt(base::to_address(browser_),
                   chrome::GetNewTabURL(base::to_address(browser_)),
                   /*index=*/-1, /*foreground=*/true,
                   /*group=*/std::nullopt,
                   /*pinned=*/false);
}

void ClosingLastTabController::MaybeHandleOnlyWebPanelLeft(
    bool tab_moved_to_other_window) {
  // Tabs may have come or gone since this task was posted.
  if (!sidebar::IsWebPanelOnlyTab(base::to_address(browser_))) {
    return;
  }

  if (ShouldCloseWindowWhenTabStripEmpty()) {
    sidebar::MaybeCloseWebPanelForShutdown(base::to_address(browser_));
    return;
  }

  if (tab_moved_to_other_window) {
    // Nothing here to replace, so the window closes despite the pref. Override
    // it only now the close is going ahead, so an abandoned attempt leaves
    // nothing behind: removing the panel's tab empties the strip, and
    // TabStripEmpty() consults the pref again.
    ignore_enable_closing_last_tab_pref_ = true;
    sidebar::MaybeCloseWebPanelForShutdown(base::to_address(browser_));
    return;
  }

  AddNewTabForEmptyTabStrip();
}

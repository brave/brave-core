/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_CLOSING_LAST_TAB_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_CLOSING_LAST_TAB_CONTROLLER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;

// Keeps a normal window open with a new tab when its last tab closes, unless
// the kEnableClosingLastTab pref says to close it.
class ClosingLastTabController : public TabStripModelObserver {
 public:
  DECLARE_USER_DATA(ClosingLastTabController);

  explicit ClosingLastTabController(BrowserWindowInterface& browser);
  ClosingLastTabController(const ClosingLastTabController&) = delete;
  ClosingLastTabController& operator=(const ClosingLastTabController&) = delete;
  ~ClosingLastTabController() override;

  // Returns the instance owned by `browser`, or nullptr.
  static ClosingLastTabController* From(BrowserWindowInterface* browser);

  // Called when the tab strip becomes empty. Returns true when the window
  // stays open, in which case a new tab is added to it.
  bool MaybeKeepWindowOpen();

  // Lets the window close once its tab strip empties, whatever the pref says,
  // e.g. when all its tabs are brought to another window.
  void set_ignore_enable_closing_last_tab_pref() {
    ignore_enable_closing_last_tab_pref_ = true;
  }

 private:
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // False when the window should stay open with a fresh tab instead.
  bool ShouldCloseWindowWhenTabStripEmpty() const;
  void AddNewTabForEmptyTabStrip();

  // The web panel is backed by a pinned tab, so a strip holding only it never
  // fires TabStripEmpty() even though the user just closed their last tab.
  // `tab_moved_to_other_window` is true when that tab was detached for
  // insertion elsewhere, leaving nothing here to replace.
  //
  // To close, this removes the panel's tab rather than closing the window, so
  // the ordinary last-tab path runs through TabStripEmpty() as if the panel
  // never existed - closing the window directly would detach the panel midway
  // through the close and re-enter it via TabStripEmpty(). The panel doesn't
  // get to veto a gesture aimed at the user's tabs, so its unload handlers are
  // skipped.
  void MaybeHandleOnlyWebPanelLeft(bool tab_moved_to_other_window);

  const raw_ref<BrowserWindowInterface> browser_;

  // When kEnableClosingLastTab is false, the window gets a new tab when its
  // tab strip empties. Some cases shouldn't add one, like when the user brings
  // all tabs to another window.
  bool ignore_enable_closing_last_tab_pref_ = false;

  ui::ScopedUnownedUserData<ClosingLastTabController> scoped_unowned_user_data_;
  base::WeakPtrFactory<ClosingLastTabController> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_CLOSING_LAST_TAB_CONTROLLER_H_

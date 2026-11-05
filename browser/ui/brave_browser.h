/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser.h"

class BraveBrowser : public Browser {
 public:
  explicit BraveBrowser(BrowserWindowCreateParams params);
  ~BraveBrowser() override;

  BraveBrowser(const BraveBrowser&) = delete;
  BraveBrowser& operator=(const BraveBrowser&) = delete;

  // Browser overrides:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void TabStripEmpty() override;

  // Allows ignoring onbeforeunload handlers when closing selected tabs.
  void SetTabsToIgnoreBeforeUnloadHandlers(
      const base::flat_set<tabs::TabHandle>& for_contents);

  // Used by the BrowserWebContentsDelegate override (see
  // brave/chromium_src/chrome/browser/ui/browser_web_contents_delegate) to
  // suppress before-unload dialogs for tabs being moved to another window.
  bool ShouldIgnoreBeforeUnloadHandlerForTab(tabs::TabHandle handle) const;

  void set_ignore_enable_closing_last_tab_pref() {
    ignore_enable_closing_last_tab_pref_ = true;
  }

 private:
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

  // When "kEnableClosingLastTab" is false, browser will try to add new tab in
  // TabStripEmpty() if there is no tab. But, in some cases, we should not add
  // new tab, like when user tries to "Bring all tabs" to other window.
  bool ignore_enable_closing_last_tab_pref_ = false;

  // WebContents for which onbeforeunload handlers should be ignored.
  base::flat_set<tabs::TabHandle> tabs_closing_with_onbeforeunload_ignore_;

  base::WeakPtrFactory<BraveBrowser> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

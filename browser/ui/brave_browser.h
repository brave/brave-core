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
  explicit BraveBrowser(const CreateParams& params);
  ~BraveBrowser() override;

  BraveBrowser(const BraveBrowser&) = delete;
  BraveBrowser& operator=(const BraveBrowser&) = delete;

  // Browser overrides:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void OnTabClosing(tabs::TabInterface* tab,
                    bool* had_active_modal_dialog) override;
  void TabStripEmpty() override;

  // Returns true when we should ask browser closing to users before handling
  // any warning/onbeforeunload handlers.
  bool ShouldAskForBrowserClosingBeforeHandlers();

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
  friend class BraveTestLauncherDelegate;
  friend class WindowClosingConfirmBrowserTest;
  friend class InProcessBrowserTest;

  // static
  static void SuppressBrowserWindowClosingDialogForTesting(bool suppress);

  bool AreAllTabsSharedPinnedTabs();

  // When "kEnableClosingLastTab" is false, browser will try to add new tab in
  // TabStripEmpty() if there is no tab. But, in some cases, we should not add
  // new tab, like when user tries to "Bring all tabs" to other window.
  bool ignore_enable_closing_last_tab_pref_ = false;

  // WebContents for which onbeforeunload handlers should be ignored.
  base::flat_set<tabs::TabHandle> tabs_closing_with_onbeforeunload_ignore_;

  base::WeakPtrFactory<BraveBrowser> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

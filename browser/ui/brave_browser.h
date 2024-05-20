/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser.h"

#if defined(TOOLKIT_VIEWS)
namespace sidebar {
class SidebarController;
}  // namespace sidebar
#endif

class BraveBrowserWindow;

namespace content {
class WebContents;
}  // namespace content

class BraveBrowser : public Browser {
 public:
  explicit BraveBrowser(const CreateParams& params);
  ~BraveBrowser() override;

  BraveBrowser(const BraveBrowser&) = delete;
  BraveBrowser& operator=(const BraveBrowser&) = delete;

  static bool ShouldUseBraveWebViewRoundedCorners(Browser* browser);

  // Browser overrides:
  void ScheduleUIUpdate(content::WebContents* source,
                        unsigned changed_flags) override;
  bool ShouldDisplayFavicon(content::WebContents* web_contents) const override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void FinishWarnBeforeClosing(WarnBeforeClosingResult result) override;
  void BeforeUnloadFired(content::WebContents* source,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  bool TryToCloseWindow(
      bool skip_beforeunload,
      const base::RepeatingCallback<void(bool)>& on_close_confirmed) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  void ResetTryToCloseWindow() override;

  void OnTabClosing(content::WebContents* contents) override;
  void TabStripEmpty() override;

  // Returns true when we should ask browser closing to users before handling
  // any warning/onbeforeunload handlers.
  bool ShouldAskForBrowserClosingBeforeHandlers();

#if defined(TOOLKIT_VIEWS)
  sidebar::SidebarController* sidebar_controller() {
    return sidebar_controller_.get();
  }
#endif

  BraveBrowserWindow* brave_window();

  void set_confirmed_to_close(bool close) { confirmed_to_close_ = close; }

 private:
  friend class BraveTestLauncherDelegate;
  friend class WindowClosingConfirmBrowserTest;
  friend class InProcessBrowserTest;

  // static
  static void SuppressBrowserWindowClosingDialogForTesting(bool suppress);

  bool AreAllTabsSharedPinnedTabs();

  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;

  // Set true when user allowed to close browser before starting any
  // warning or onbeforeunload handlers.
  bool confirmed_to_close_ = false;

  base::WeakPtrFactory<BraveBrowser> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

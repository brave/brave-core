/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

#include <memory>

#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"

#if BUILDFLAG(ENABLE_SIDEBAR)
namespace sidebar {
class SidebarController;
}  // namespace sidebar

class BraveBrowserWindow;
#endif

namespace content {
class WebContents;
}  // namespace content

class BraveBrowser : public Browser {
 public:
  explicit BraveBrowser(const CreateParams& params);
  ~BraveBrowser() override;

  BraveBrowser(const BraveBrowser&) = delete;
  BraveBrowser& operator=(const BraveBrowser&) = delete;

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
  void ResetTryToCloseWindow() override;

  void TabStripEmpty() override;
  // Returns true when we should ask browser closing to users before handling
  // any warning/onbeforeunload handlers.
  bool ShouldAskForBrowserClosingBeforeHandlers();

#if BUILDFLAG(ENABLE_SIDEBAR)
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

#if BUILDFLAG(ENABLE_SIDEBAR)
  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;
#endif

  // Set true when user allowed to close browser before starting any
  // warning or onbeforeunload handlers.
  bool confirmed_to_close_ = false;
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

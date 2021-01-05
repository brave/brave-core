/* Copyright (c) 2020 The Brave Authors. All rights reserved.
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

class BraveBrowser : public Browser {
 public:
  explicit BraveBrowser(const CreateParams& params);
  ~BraveBrowser() override;

  BraveBrowser(const BraveBrowser&) = delete;
  BraveBrowser& operator=(const BraveBrowser&) = delete;

  // Browser overrides:
  void ScheduleUIUpdate(content::WebContents* source,
                        unsigned changed_flags) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

#if BUILDFLAG(ENABLE_SIDEBAR)
  sidebar::SidebarController* sidebar_controller() {
    return sidebar_controller_.get();
  }
#endif

  BraveBrowserWindow* brave_window();

 private:
#if BUILDFLAG(ENABLE_SIDEBAR)
  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;
#endif
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_H_

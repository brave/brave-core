/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_BEFORE_UNLOAD_IGNORE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_BEFORE_UNLOAD_IGNORE_CONTROLLER_H_

#include "base/containers/flat_set.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BrowserWindowInterface;

// Tracks the tabs of a window that close without running their onbeforeunload
// handlers, e.g. when Shields shreds a site's tabs.
class BeforeUnloadIgnoreController : public TabStripModelObserver {
 public:
  DECLARE_USER_DATA(BeforeUnloadIgnoreController);

  explicit BeforeUnloadIgnoreController(BrowserWindowInterface& browser);
  BeforeUnloadIgnoreController(const BeforeUnloadIgnoreController&) = delete;
  BeforeUnloadIgnoreController& operator=(const BeforeUnloadIgnoreController&) =
      delete;
  ~BeforeUnloadIgnoreController() override;

  // Returns the instance owned by `browser`, or nullptr.
  static BeforeUnloadIgnoreController* From(BrowserWindowInterface* browser);

  // Replaces the tabs whose onbeforeunload handlers are ignored. A tab stops
  // being ignored once it leaves the tab strip.
  void SetTabsToIgnoreBeforeUnloadHandlers(
      const base::flat_set<tabs::TabHandle>& tabs);

  bool ShouldIgnoreBeforeUnloadHandlerForTab(tabs::TabHandle handle) const;

 private:
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  base::flat_set<tabs::TabHandle> tabs_to_ignore_;

  ui::ScopedUnownedUserData<BeforeUnloadIgnoreController>
      scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_BEFORE_UNLOAD_IGNORE_CONTROLLER_H_

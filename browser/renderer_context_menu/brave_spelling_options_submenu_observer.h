/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_RENDERER_CONTEXT_MENU_SPELLING_OPTIONS_SUBMENU_OBSERVER_H_
#define BRAVE_BROWSER_RENDERER_CONTEXT_MENU_SPELLING_OPTIONS_SUBMENU_OBSERVER_H_

#include "chrome/browser/renderer_context_menu/spelling_options_submenu_observer.h"

// Subclass SpellingOptionsSubMenuObserver to override InitMenu so that we can
// remove extaneous separator and disable the submenu if it ends up empty.
class BraveSpellingOptionsSubMenuObserver
    : public SpellingOptionsSubMenuObserver {
 public:
  BraveSpellingOptionsSubMenuObserver(RenderViewContextMenuProxy* proxy,
                                      ui::SimpleMenuModel::Delegate* delegate,
                                      int group_id);
  void InitMenu(const content::ContextMenuParams& params) override;

  // The following enum and method are only used for testing.
  enum GtestMode {
    GTEST_MODE_DISABLED,
    GTEST_MODE_NORMAL,
    GTEST_MODE_EMPTY_SUBMENU,
  };
  void SetGtestMode(GtestMode mode = GTEST_MODE_NORMAL);

 private:
  GtestMode gtest_mode_;
};

#endif  // BRAVE_BROWSER_RENDERER_CONTEXT_MENU_SPELLING_OPTIONS_SUBMENU_OBSERVER_H_

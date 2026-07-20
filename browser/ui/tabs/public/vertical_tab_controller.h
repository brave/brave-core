// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

class FocusModeController;
class PrefService;

class VerticalTabController {
 public:
  // Convenience method to get VerticalTabController from
  // BrowserWindowInterface. Returns nullptr if the browser window is nullptr.
  static VerticalTabController* FromBrowser(BrowserWindowInterface* browser);
  static const VerticalTabController* FromBrowser(
      const BrowserWindowInterface* browser);

  VerticalTabController(BrowserWindowInterface::Type type,
                        PrefService* prefs,
                        FocusModeController* focus_mode_controller);
  VerticalTabController(const VerticalTabController&) = delete;
  VerticalTabController& operator=(const VerticalTabController&) = delete;
  ~VerticalTabController();

  // Returns true if the current |browser| might ever support vertical tabs.
  bool SupportsBraveVerticalTabs() const;

  // Returns true when users chose to use vertical tabs
  bool ShouldShowBraveVerticalTabs() const;

  // Returns true when we should show window title on window frame when vertical
  // tab strip is enabled.
  bool ShouldShowWindowTitleForVerticalTabs() const;

  // Returns true if we should trigger floating vertical tab strip on mouse
  // over.
  bool IsFloatingVerticalTabsEnabled() const;

  bool IsVerticalTabOnRight() const;

  // Returns true if we should hide vertical tab completely when collapsed. In
  // this case vertical tab strip will be almost a few pixels wide vertical bar
  // when collapsed. And will expand to show tab icons when mouse is over the
  // bar regardless of the setting of kVerticalTabsFloatingEnabled.
  bool ShouldHideVerticalTabsCompletelyWhenCollapsed() const;

  // Returns true when the vertical tab toggle button (the button used to
  // expand/collapse the vertical tab strip) should be shown.
  bool ShouldShowVerticalTabToggleButton() const;

 private:
  BrowserWindowInterface::Type type_;
  raw_ptr<PrefService> prefs_;
  raw_ptr<FocusModeController> focus_mode_controller_;
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_VERTICAL_TAB_CONTROLLER_H_

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

class Browser;
class Profile;

class BraveLocationBarView : public LocationBarView {
 public:
  BraveLocationBarView(Browser* browser,
                       Profile* profile,
                       CommandUpdater* command_updater,
                       Delegate* delegate,
                       bool is_popup_mode);
  ~BraveLocationBarView() override;

  // LocationBarView overrides:
  void OnOmniboxBlurred() override;

  // Brave-specific methods
  void SetTemporaryVisibilityInFullscreen(bool visible);

 private:
  // Tracks if we're showing the location bar temporarily in fullscreen
  bool is_temporarily_visible_in_fullscreen_ = false;

  DISALLOW_COPY_AND_ASSIGN(BraveLocationBarView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
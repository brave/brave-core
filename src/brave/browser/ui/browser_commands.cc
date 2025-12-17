// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/browser_commands.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

namespace brave {

void FocusLocationBarInFullscreen(Browser* browser) {
  if (!browser || !browser->window() || !browser->window()->IsFullscreen())
    return;

  LocationBar* location_bar = browser->window()->GetLocationBar();
  if (!location_bar)
    return;

  BraveLocationBarView* brave_location_bar = 
      static_cast<BraveLocationBarView*>(location_bar);
  
  if (brave_location_bar) {
    brave_location_bar->SetTemporaryVisibilityInFullscreen(true);
    location_bar->FocusLocation(false);
  }
}

}  // namespace brave
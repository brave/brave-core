/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COCOA_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_MAC_H_
#define BRAVE_BROWSER_UI_COCOA_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_MAC_H_

#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"

class BraveLocationBarViewMac : public LocationBarViewMac {
 public:
  BraveLocationBarViewMac(AutocompleteTextField* field,
                          CommandUpdater* command_updater,
                          Profile* profile,
                          Browser* browser);
  ~BraveLocationBarViewMac() override;
  PageInfoVerboseType GetPageInfoVerboseType() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveLocationBarViewMac);
};

#endif  // BRAVE_BROWSER_UI_COCOA_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_MAC_H_

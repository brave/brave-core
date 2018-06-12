/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/ui/cocoa/location_bar/brave_location_bar_view_mac.h"

#include "brave/common/url_constants.h"

BraveLocationBarViewMac::BraveLocationBarViewMac(AutocompleteTextField* field,
                                                 CommandUpdater* command_updater,
                                                 Profile* profile,
                                                 Browser* browser) :
    LocationBarViewMac(field, command_updater, profile, browser) {
}

BraveLocationBarViewMac::~BraveLocationBarViewMac() {
}

PageInfoVerboseType BraveLocationBarViewMac::GetPageInfoVerboseType() const {
  if (GetToolbarModel()->GetURL().SchemeIs(kBraveUIScheme)) {
    return PageInfoVerboseType::kChrome;
  }
  return LocationBarViewMac::GetPageInfoVerboseType();
}

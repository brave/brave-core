/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/location_bar/brave_location_bar.h"


// Remove Chromium's original bookmark button, in favor
// of Brave's own bookmark button.
bool BraveLocationBar::IsBookmarkStarHiddenByExtension() const {
  return true;
}
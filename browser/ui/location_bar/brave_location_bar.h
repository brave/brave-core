/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_LOCATION_BAR_BRAVE_LOCATION_BAR_H_
#define BRAVE_BROWSER_UI_LOCATION_BAR_BRAVE_LOCATION_BAR_H_

#include "chrome/browser/ui/location_bar/location_bar.h"

class BraveLocationBar : public LocationBar {
  public:
    using LocationBar::LocationBar;
  protected:
    bool IsBookmarkStarHiddenByExtension() const override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveLocationBar);
};

#endif
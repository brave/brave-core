/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_BROWSER_MAIN_EXTRA_PARTS_VIEWS_LINUX_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_BROWSER_MAIN_EXTRA_PARTS_VIEWS_LINUX_H_

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.h"

class BraveBrowserMainExtraPartsViewsLinux
                                : public ChromeBrowserMainExtraPartsViewsLinux {
  public:
    using ChromeBrowserMainExtraPartsViewsLinux::ChromeBrowserMainExtraPartsViewsLinux;
    void PreEarlyInitialization() override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveBrowserMainExtraPartsViewsLinux);
};

#endif

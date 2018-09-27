/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_

#include "chrome/browser/ui/views/frame/browser_view_layout.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

class BraveBrowserViewLayout : public BrowserViewLayout {
  public:
    using BrowserViewLayout::BrowserViewLayout;
    // views::LayoutManager overrides:
    void Layout(views::View* host) override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveBrowserViewLayout);
};

#endif
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_H_

#include "chrome/browser/ui/views/frame/browser_view.h"

class BraveBrowserView : public BrowserView {
  public:
    using BrowserView::BrowserView;
    void SetStarredState(bool is_starred) override;
    void OnThemeChanged() override;
  private:
    DISALLOW_COPY_AND_ASSIGN(BraveBrowserView);
};

#endif
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_

#include "chrome/browser/ui/views/frame/browser_frame.h"

class BrowserView;

class BraveBrowserFrame : public BrowserFrame {
  public:
    explicit BraveBrowserFrame(BrowserView* browser_view);
    ~BraveBrowserFrame() override;
    const ui::NativeTheme* GetNativeTheme() const override;
    void ThemeChanged() override;

  private:
    // The BrowserView is our ClientView. This is a pointer to it.
    BrowserView* browser_view_;

    DISALLOW_COPY_AND_ASSIGN(BraveBrowserFrame);
};

#endif

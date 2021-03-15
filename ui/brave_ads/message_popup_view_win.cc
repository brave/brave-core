/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_ads/message_popup_view.h"

#include <windows.h>

#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

namespace brave_ads {

gfx::Rect MessagePopupView::GetVisibleFrameForPrimaryDisplay() const {
  RECT rect;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
  return display::Screen::GetScreen()->ScreenToDIPRectInWindow(nullptr,
                                                               gfx::Rect(rect));
}

}  // namespace brave_ads

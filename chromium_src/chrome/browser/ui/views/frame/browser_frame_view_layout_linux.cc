/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/features.h"

#include "src/chrome/browser/ui/views/frame/browser_frame_view_layout_linux.cc"

int BrowserFrameViewLayoutLinux::NonClientTopHeight(bool restored) const {
  if (tabs::features::ShouldShowVerticalTabs()) {
    // Set minimum height to show caption buttons.
    return 50;
  }

  return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
}

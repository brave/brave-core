/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"

int VerticalTabStripWidgetDelegateView::GetVerticalTabStripCornerRadiusMac()
    const {
  if (@available(macOS 26, *)) {
    return 20;
  }
  return 8;
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/ui/views/widget/widget.cc"

#if BUILDFLAG(IS_MAC)

#include "ui/views/widget/native_widget_mac.h"

namespace views {

void Widget::SetWindowTitleVisibility(bool visible) {
  static_cast<NativeWidgetMac*>(native_widget_private())
      ->SetWindowTitleVisibility(visible);
}

void Widget::ResetWindowControlsPosition() {
  static_cast<NativeWidgetMac*>(native_widget_private())
      ->ResetWindowControlsPosition();
}

void Widget::UpdateWindowTitleColor(SkColor color) {
  static_cast<NativeWidgetMac*>(native_widget_private())
      ->UpdateWindowTitleColor(color);
}

}  // namespace views

#endif  // BUILDFLAG(IS_MAC)

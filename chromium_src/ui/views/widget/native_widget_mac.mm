/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/ui/views/widget/native_widget_mac.mm"

namespace views {

void NativeWidgetMac::SetWindowTitleVisibility(bool visible) {
  if (overridden_window_title_visibility_.has_value() &&
      visible == *overridden_window_title_visibility_) {
    return;
  }

  GetNSWindowMojo()->SetWindowTitleVisibility(visible);
  overridden_window_title_visibility_ = visible;
}

void NativeWidgetMac::UpdateWindowTitleColor(SkColor color) {
  GetNSWindowMojo()->UpdateWindowTitleColor(color);
}

bool NativeWidgetMac::GetOverriddenWindowTitleVisibility() const {
  DCHECK(has_overridden_window_title_visibility())
      << "Didn't call SetWindowTitleVisibility(). Use "
         "WidgetDelegate::ShouldShowWindowTitle() instead.";
  return *overridden_window_title_visibility_;
}

void NativeWidgetMac::ResetWindowControlsPosition() {
  GetNSWindowMojo()->ResetWindowControlsPosition();
}

}  // namespace views

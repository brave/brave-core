/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/ui/views/cocoa/native_widget_mac_ns_window_host.mm"

namespace views {

void NativeWidgetMacNSWindowHost::SetWindowTitleVisibility(bool visible) {
  GetNSWindowMojo()->SetWindowTitleVisibility(visible);
}

}  // namespace views

/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/update_recommended_message_box_mac.h"

#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"
#include "components/constrained_window/constrained_window_views.h"

// static
void UpdateRecommendedMessageBoxMac::Show(gfx::NativeWindow parent_window) {
  // When the window closes, it will delete itself.
  constrained_window::CreateBrowserModalDialogViews(
      new UpdateRecommendedMessageBoxMac(), parent_window)->Show();
}

bool UpdateRecommendedMessageBoxMac::Accept() {
  return brave_relaunch_handler::RelaunchOnMac() ||
         UpdateRecommendedMessageBox::Accept();
}

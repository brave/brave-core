/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/update_recommended_message_box_mac.h"

#import "brave/browser/mac/sparkle_glue.h"
#include "components/constrained_window/constrained_window_views.h"

// static
void UpdateRecommendedMessageBoxMac::Show(gfx::NativeWindow parent_window) {
  // When the window closes, it will delete itself.
  constrained_window::CreateBrowserModalDialogViews(
      new UpdateRecommendedMessageBoxMac(), parent_window)->Show();
}

bool UpdateRecommendedMessageBoxMac::Accept() {
  [[SparkleGlue sharedSparkleGlue] relaunch];
  return true;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"

#if defined(OS_MACOSX)
#include "brave/browser/ui/views/update_recommended_message_box_mac.h"
#endif

void BraveBrowserView::SetStarredState(bool is_starred) {
  BookmarkButton* button = ((BraveToolbarView*)toolbar())->bookmark_button();
  if (button)
    button->SetToggled(is_starred);
}

void BraveBrowserView::ShowUpdateChromeDialog() {
#if defined(OS_MACOSX)
  // On mac, sparkle frameworks's relaunch api is used.
  UpdateRecommendedMessageBoxMac::Show(GetNativeWindow());
#else
  BrowserView::ShowUpdateChromeDialog();
#endif
}

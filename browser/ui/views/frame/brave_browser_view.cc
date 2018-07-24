/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"

void BraveBrowserView::SetStarredState(bool is_starred) {
  BookmarkButton* button = ((BraveToolbarView *)toolbar())->bookmark_button();
  if (button)
    button->SetToggled(is_starred);
}
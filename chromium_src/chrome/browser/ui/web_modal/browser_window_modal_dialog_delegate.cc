/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/tabs/public/tab_interface.h"

#include <chrome/browser/ui/web_modal/browser_window_modal_dialog_delegate.cc>

// Overrides ChromeWebModalDialogManagerDelegate::IsWebContentsVisible() to
// suppress web modal dialogs on inactive split tabs. A split tab can be
// visible (i.e. rendered) but not activated, and without this override a modal
// dialog from the inactive pane would incorrectly appear.
bool BrowserWindowModalDialogDelegate::IsWebContentsVisible(
    content::WebContents* web_contents) {
  const bool original_visible =
      ChromeWebModalDialogManagerDelegate::IsWebContentsVisible(web_contents);
  auto* tab = tabs::TabInterface::MaybeGetFromContents(web_contents);
  if (!tab) {
    return original_visible;
  }
  if (original_visible && !tab->IsActivated()) {
    return false;
  }
  return original_visible;
}

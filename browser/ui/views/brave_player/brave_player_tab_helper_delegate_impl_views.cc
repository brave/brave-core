/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_player/brave_player_tab_helper_delegate_impl.h"

#include "brave/browser/ui/views/brave_player/ad_block_adjustment_dialog.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"

void BravePlayerTabHelperDelegateImpl::ShowAdBlockAdjustmentSuggestion(
    content::WebContents* contents) {
  constrained_window::ShowWebModalDialogViews(
      new AdBlockAdjustmentDialog(contents->GetLastCommittedURL()), contents);
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_WEB_CONTENTS_MODAL_DIALOG_MANAGER_ON_VISIBILITY_CHANGED    \
  if (web_contents_visibility_ != content::Visibility::HIDDEN &&         \
      (delegate_ && !delegate_->IsWebContentsVisible(web_contents()))) { \
    web_contents_visibility_ = content::Visibility::HIDDEN;              \
  }

#include "src/components/web_modal/web_contents_modal_dialog_manager.cc"

#undef BRAVE_WEB_CONTENTS_MODAL_DIALOG_MANAGER_ON_VISIBILITY_CHANGED

namespace web_modal {

void WebContentsModalDialogManager::OnTabActiveStateChanged() {
  // OnVisibilityChanged() updates dialog show/hide state based on tab's
  // activation state.
  OnVisibilityChanged(web_contents()->GetVisibility());
}

}  // namespace web_modal

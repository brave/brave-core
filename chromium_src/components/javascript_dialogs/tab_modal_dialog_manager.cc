/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// If tab's web contents is not active(not foremost), it should be
// treated as hidden. This situation can happen from inactive split tab.
// Otherwise, dialog could be launched from inactive split tab.
#define BRAVE_TAB_MODAL_DIALOG_MANAGER_ON_VISIBILITY_CHANGED \
  if (visibility != content::Visibility::HIDDEN &&           \
      !delegate_->IsWebContentsForemost()) {                 \
    visibility = content::Visibility::HIDDEN;                \
  }

#include "src/components/javascript_dialogs/tab_modal_dialog_manager.cc"

#undef BRAVE_TAB_MODAL_DIALOG_MANAGER_ON_VISIBILITY_CHANGED

namespace javascript_dialogs {

void TabModalDialogManager::OnTabActiveStateChanged() {
  OnVisibilityChanged(web_contents()->GetVisibility());
}

}  // namespace javascript_dialogs

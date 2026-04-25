/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.h"

#define UpdateActionState UpdateActionState_UnUsed
#define ShowActionEphemerallyInToolbar \
  ShowActionEphemerallyInToolbar_ChromiumImpl

#include <chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.cc>
#undef ShowActionEphemerallyInToolbar
#undef UpdateActionState

void PinnedToolbarActionsContainer::UpdateActionState(actions::ActionId id,
                                                      bool is_active) {
  // We don't want anything pinned. Downloads button is shown ephemerally on
  // download status change.
  return;
}

void PinnedToolbarActionsContainer::ShowActionEphemerallyInToolbar(
    actions::ActionId id,
    bool show) {
  if (id != kActionShowDownloads && id != kActionSendTabToSelf) {
    return;
  }
  PinnedToolbarActionsContainer::ShowActionEphemerallyInToolbar_ChromiumImpl(
      id, show);
}

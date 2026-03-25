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
  // We don't want regular actions pinned/updated. However, keep Chromium's
  // active-state handling for ephemeral actions so the same button can toggle
  // its bubble/popup closed on a second click.
  if (id != kActionShowDownloads && id != kActionSendTabToSelf) {
    return;
  }

  PinnedToolbarActionsContainer::UpdateActionState_UnUsed(id, is_active);
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

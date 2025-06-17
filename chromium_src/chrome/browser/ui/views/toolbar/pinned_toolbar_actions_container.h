/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_TOOLBAR_ACTIONS_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_TOOLBAR_ACTIONS_CONTAINER_H_

// We only want to allow the downloads button to be show in the pinned toolbar
// container for now.
#define UpdateActionState(...)           \
  UpdateActionState_UnUsed(__VA_ARGS__); \
  void UpdateActionState(__VA_ARGS__)

#define ShowActionEphemerallyInToolbar(...)                 \
  ShowActionEphemerallyInToolbar_ChromiumImpl(__VA_ARGS__); \
  void ShowActionEphemerallyInToolbar(__VA_ARGS__)

#include <chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.h>  // IWYU pragma: export
#undef ShowActionEphemerallyInToolbar
#undef UpdateActionState

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_TOOLBAR_ACTIONS_CONTAINER_H_

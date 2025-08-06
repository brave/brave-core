/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_ACTION_TOOLBAR_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_ACTION_TOOLBAR_BUTTON_H_

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

// We don't want to show context menu for pinned container buttons. For now we
// only allow downloads button and we don't want to show any customization UI
// elements yet.
#define ShouldShowEphemerallyInToolbar(...)    \
  ShouldShowEphemerallyInToolbar(__VA_ARGS__); \
  bool ShouldShowMenu() override

// Allow ToolbarButton's color override to persist.
#define UpdateIcon           \
  UpdateIcon_ChromiumImpl(); \
  void UpdateIcon

#include <chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h>  // IWYU pragma: export
#undef UpdateIcon
#undef ShouldShowEphemerallyInToolbar

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_PINNED_ACTION_TOOLBAR_BUTTON_H_

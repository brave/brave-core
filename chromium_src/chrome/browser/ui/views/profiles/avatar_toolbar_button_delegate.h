/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

#define GetAvatarIcon virtual GetAvatarIcon
#define GetAvatarTooltipText virtual GetAvatarTooltipText
// Adding friend class to `BraveAvatarToolbarButton` to still allow its instance
// to call `GetWindowCount`.
#define GetWindowCount                   \
  GetWindowCountUnused() const;          \
  friend class BraveAvatarToolbarButton; \
  int GetWindowCount
#include "src/chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"  // IWYU pragma: export
#undef GetAvatarIcon
#undef GetAvatarTooltipText
#undef GetWindowCount

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

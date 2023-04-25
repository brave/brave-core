/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button_delegate.h"

BraveAvatarToolbarButtonDelegate::BraveAvatarToolbarButtonDelegate(
    AvatarToolbarButton* button,
    Browser* browser)
    : AvatarToolbarButtonDelegate(button, browser) {}

gfx::Image BraveAvatarToolbarButtonDelegate::GetGaiaAccountImage() const {
  return gfx::Image();
}

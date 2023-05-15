/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"

class BraveAvatarToolbarButtonDelegate : public AvatarToolbarButtonDelegate {
 public:
  using AvatarToolbarButtonDelegate::AvatarToolbarButtonDelegate;
  BraveAvatarToolbarButtonDelegate(AvatarToolbarButton* button,
                                   Browser* browser);
  BraveAvatarToolbarButtonDelegate(const BraveAvatarToolbarButtonDelegate&) =
      delete;
  BraveAvatarToolbarButtonDelegate& operator=(
      const BraveAvatarToolbarButtonDelegate&) = delete;
  ~BraveAvatarToolbarButtonDelegate() override = default;

  gfx::Image GetGaiaAccountImage() const;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

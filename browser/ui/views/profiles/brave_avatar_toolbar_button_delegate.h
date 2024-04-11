/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"

class BraveAvatarToolbarButtonDelegate final
    : public AvatarToolbarButtonDelegate {
 public:
  using AvatarToolbarButtonDelegate::AvatarToolbarButtonDelegate;
  BraveAvatarToolbarButtonDelegate(AvatarToolbarButton* button,
                                   Browser* browser);
  BraveAvatarToolbarButtonDelegate(const BraveAvatarToolbarButtonDelegate&) =
      delete;
  BraveAvatarToolbarButtonDelegate& operator=(
      const BraveAvatarToolbarButtonDelegate&) = delete;

  gfx::Image GetGaiaAccountImage() const;

  // Overrides from AvatarToolbarButtonDelegate:
  ui::ImageModel GetAvatarIcon(int icon_size,
                               SkColor icon_color) const override;
  std::u16string GetAvatarTooltipText() const override;

 private:
  raw_ptr<Browser> browser_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

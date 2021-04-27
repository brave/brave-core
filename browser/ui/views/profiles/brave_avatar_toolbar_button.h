/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"

class BrowserView;

class BraveAvatarToolbarButton : public AvatarToolbarButton {
 public:
  explicit BraveAvatarToolbarButton(BrowserView* browser_view);

  // ToolbarButton:
  void SetHighlight(const std::u16string& highlight_text,
                    base::Optional<SkColor> highlight_color) override;
  void UpdateColorsAndInsets() override;

 private:
  // AvatarToolbarButton:
  ui::ImageModel GetAvatarIcon(
      ButtonState state,
      const gfx::Image& profile_identity_image) const override;
  std::u16string GetAvatarTooltipText() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveAvatarToolbarButton);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_

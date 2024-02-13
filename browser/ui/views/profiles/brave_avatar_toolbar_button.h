/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_

#include <optional>

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BrowserView;

class BraveAvatarToolbarButton : public AvatarToolbarButton {
  METADATA_HEADER(BraveAvatarToolbarButton, AvatarToolbarButton)

 public:
  explicit BraveAvatarToolbarButton(BrowserView* browser_view);
  BraveAvatarToolbarButton(const BraveAvatarToolbarButton&) = delete;
  BraveAvatarToolbarButton& operator=(const BraveAvatarToolbarButton&) = delete;
  ~BraveAvatarToolbarButton() override;

  AvatarToolbarButton::State GetAvatarButtonState() const;

  // ToolbarButton:
  void SetHighlight(const std::u16string& highlight_text,
                    std::optional<SkColor> highlight_color) override;
  void UpdateColorsAndInsets() override;
  void OnThemeChanged() override;

 private:
  // AvatarToolbarButton:
  ui::ImageModel GetAvatarIcon(
      ButtonState state,
      const gfx::Image& profile_identity_image) const override;
  std::u16string GetAvatarTooltipText() const override;
  int GetWindowCount() const;

  base::WeakPtrFactory<BraveAvatarToolbarButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_

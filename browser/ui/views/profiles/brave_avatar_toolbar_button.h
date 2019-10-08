/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_AVATAR_TOOLBAR_BUTTON_H_

#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"

class BraveAvatarToolbarButton : public AvatarToolbarButton {
  public:
    using AvatarToolbarButton::AvatarToolbarButton;
  private:
    gfx::ImageSkia GetAvatarIcon(const gfx::Image& gaia_image) const override;
    DISALLOW_COPY_AND_ASSIGN(BraveAvatarToolbarButton);
};

#endif
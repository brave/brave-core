/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

// Pull in all original includes: Init and GetState are too common - we don't
// want to redefine them elsewhere.
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "ui/gfx/image/image.h"

#define GetState virtual GetState
#include "src/chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"
#undef GetState

class BraveAvatarToolbarButtonDelegate : public AvatarToolbarButtonDelegate {
 public:
  using AvatarToolbarButtonDelegate::AvatarToolbarButtonDelegate;
  BraveAvatarToolbarButtonDelegate(AvatarToolbarButton* button,
                                   Profile* profile);
  BraveAvatarToolbarButtonDelegate(const BraveAvatarToolbarButtonDelegate&) =
      delete;
  BraveAvatarToolbarButtonDelegate& operator=(
      const BraveAvatarToolbarButtonDelegate&) = delete;
  ~BraveAvatarToolbarButtonDelegate() override = default;

  gfx::Image GetGaiaAccountImage() const;
  AvatarToolbarButton::State GetState() const override;

 private:
  Profile* const profile_ = nullptr;
};

#undef BRAVE_PROFILE_MENU_VIEW_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_AVATAR_TOOLBAR_BUTTON_DELEGATE_H_

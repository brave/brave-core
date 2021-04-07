/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"


#include "../../../../../../../chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.cc"

void BraveAvatarToolbarButtonDelegate::Init(AvatarToolbarButton* button,
                                            Profile* profile) {
  profile_ = profile;
  AvatarToolbarButtonDelegate::Init(button, profile);
}

AvatarToolbarButton::State BraveAvatarToolbarButtonDelegate::GetState() const {
  AvatarToolbarButton::State state = AvatarToolbarButtonDelegate::GetState();
  if (state == AvatarToolbarButton::State::kGenericProfile) {
    ProfileAttributesEntry* entry =
        g_browser_process->profile_manager()
            ->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile_->GetPath());
    if (entry &&
        entry->GetAvatarIconIndex() == profiles::GetPlaceholderAvatarIndex()) {
      return AvatarToolbarButton::State::kNormal;
    }
  }
  return state;
}

gfx::Image BraveAvatarToolbarButtonDelegate::GetGaiaAccountImage() const {
  return gfx::Image();
}

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button_delegate.h"

#define GET_AVATAR_TOOLTIP_TEXT_    \
  if (browser_->profile()->IsTor()) \
    return l10n_util::GetStringUTF16(IDS_TOR_PROFILE_NAME);

#define AvatarToolbarButtonDelegate BraveAvatarToolbarButtonDelegate

#include "../../../../../../../chrome/browser/ui/views/profiles/avatar_toolbar_button.cc"
#undef AvatarToolbarButtonDelegate
#undef GET_AVATAR_TOOLTIP_TEXT_

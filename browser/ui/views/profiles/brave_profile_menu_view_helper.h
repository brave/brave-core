/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_HELPER_H_

#include "base/strings/string16.h"

class Profile;

namespace gfx {
class ImageSkia;
}  // namespace gfx

namespace brave {

bool ShouldShowTorProfileButton(Profile* profile);
gfx::ImageSkia CreateTorProfileButtonIcon();
base::string16 CreateTorProfileButtonText();

}  // brave

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_HELPER_H_

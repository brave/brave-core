/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"
#include "chrome/grit/generated_resources.h"

#define BRAVE_BUILDMENU_ADD_TOR_BUTTON \
  static_cast<BraveIncognitoMenuView*>(this)->AddTorButton();

#undef IDS_INCOGNITO_PROFILE_MENU_TITLE
#define IDS_INCOGNITO_PROFILE_MENU_TITLE \
  static_cast<BraveIncognitoMenuView*>(this)->GetProfileMenuTitleId()

#undef IDS_INCOGNITO_PROFILE_MENU_CLOSE_BUTTON
#define IDS_INCOGNITO_PROFILE_MENU_CLOSE_BUTTON \
  static_cast<BraveIncognitoMenuView*>(this)->GetProfileMenuCloseButtonTextId()

#include "../../../../../../../chrome/browser/ui/views/profiles/incognito_menu_view.cc"  // NOLINT
#undef BRAVE_BUILDMENU_ADD_TOR_BUTTON
#undef IDS_INCOGNITO_PROFILE_MENU_TITLE
#undef IDS_INCOGNITO_PROFILE_MENU_CLOSE_BUTTON

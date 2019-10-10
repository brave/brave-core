/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#define BRAVE_BUILDMENU_ADD_TOR_BUTTON \
  static_cast<BraveIncognitoMenuView*>(this)->AddTorButton();

#include "../../../../../../../chrome/browser/ui/views/profiles/incognito_menu_view.cc"  // NOLINT
#undef BRAVE_BUILDMENU_ADD_TOR_BUTTON

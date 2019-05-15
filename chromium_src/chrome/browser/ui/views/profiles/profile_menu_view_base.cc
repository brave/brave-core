/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"
#include "brave/browser/ui/views/profiles/brave_profile_chooser_view.h"

#include "chrome/browser/ui/views/profiles/incognito_menu_view.h"
#include "chrome/browser/ui/views/profiles/profile_chooser_view.h"

#define IncognitoMenuView BraveIncognitoMenuView
#define ProfileChooserView BraveProfileChooserView
#include "../../../../../../../chrome/browser/ui/views/profiles/profile_menu_view_base.cc"  // NOLINT
#undef IncognitoMenuView
#undef ProfileChooserView

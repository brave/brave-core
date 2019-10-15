/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/profiles/brave_profile_menu_view.h"

#define BRAVE_ADDOPTIONSVIEW_ADD_TOR_EXIT_BUTTON_ \
  brave::IsTorProfile(browser()->profile()) ?     \
    l10n_util::GetStringUTF16(IDS_PROFILES_EXIT_TOR) :

#include "../../../../../../../chrome/browser/ui/views/profiles/profile_menu_view.cc"  // NOLINT
#undef BRAVE_ADDOPTIONSVIEW_ADD_TOR_EXIT_BUTTON_

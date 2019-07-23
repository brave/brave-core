/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/profiles/brave_profile_chooser_view.h"
#define ADD_TOR_EXIT_BUTTON_ \
  static_cast<BraveProfileChooserView*>(this)->AddTorButton(); \
  if (brave::IsTorProfile(browser()->profile())) \
    text = l10n_util::GetStringUTF16(IDS_PROFILES_EXIT_TOR);
#include "../../../../../../../chrome/browser/ui/views/profiles/profile_chooser_view.cc"  // NOLINT
#undef ADD_TOR_EXIT_BUTTON_

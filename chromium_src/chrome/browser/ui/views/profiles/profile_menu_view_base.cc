/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Continue to use profile_background_container_ to set the background color in
// the profile menu, rather than identiy_info_color_callback_
#define BRAVE_PROFILE_MENU_VIEW_BASE_SET_BACKGROUND    \
  identity_info_color_callback_ = base::DoNothing();   \
  profile_background_container_->SetBackground(        \
      views::CreateBackgroundFromPainter(              \
          views::Painter::CreateSolidRoundRectPainter( \
              background_color, /*radius=*/0, kBackgroundInsets)));

#include "src/chrome/browser/ui/views/profiles/profile_menu_view_base.cc"

#undef BRAVE_PROFILE_MENU_VIEW_BASE_SET_BACKGROUND

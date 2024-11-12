/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_BASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_BASE_H_

#define BRAVE_PROFILE_MENU_VIEW_BASE_H_  \
 private:                                \
  friend class BraveProfileMenuViewTest; \
                                         \
 public:

// Overriding this function to avoid a second call to it.
#define SetProfileIdentityInfo(...) virtual SetProfileIdentityInfo(__VA_ARGS__)

#include "src/chrome/browser/ui/views/profiles/profile_menu_view_base.h"  // IWYU pragma: export

#undef BRAVE_PROFILE_MENU_VIEW_BASE_H_
#undef SetProfileIdentityInfo

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_BASE_H_

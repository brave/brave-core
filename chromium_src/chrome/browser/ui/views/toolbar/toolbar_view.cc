/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#define LocationBarView BraveLocationBarView
#endif

#define BRAVE_TOOLBAR_VIEW_INIT                                                \
  avatar_ =                                                                    \
      AddChildView(std::make_unique<BraveAvatarToolbarButton>(browser_view_)); \
  if (false)

#include "../../../../../../../chrome/browser/ui/views/toolbar/toolbar_view.cc"
#undef BRAVE_TOOLBAR_VIEW_INIT
#if BUILDFLAG(ENABLE_EXTENSIONS)
#undef LocationBarView
#endif

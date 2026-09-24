/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// BraveAvatarToolbarButton and BraveBrowserAppMenuButton are substituted into
// ToolbarView::Init() in place of their upstream base classes via plaster, see
// brave/rewrite/chrome/browser/ui/views/toolbar/toolbar_view.cc.yaml
#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"
#include "brave/browser/ui/views/toolbar/brave_browser_app_menu_button.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
using LocationBarViewAlias = BraveLocationBarView;
#else
using LocationBarViewAlias = LocationBarView;
#endif

#include <chrome/browser/ui/views/toolbar/toolbar_view.cc>

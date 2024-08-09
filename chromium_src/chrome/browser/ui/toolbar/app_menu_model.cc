/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_bookmark_sub_menu_model.h"
#include "brave/browser/ui/toolbar/brave_recent_tabs_sub_menu_model.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/toolbar/bookmark_sub_menu_model.h"
#include "chrome/grit/generated_resources.h"

#define IDS_NEW_INCOGNITO_WINDOW_OLD IDS_NEW_INCOGNITO_WINDOW
#undef IDS_NEW_INCOGNITO_WINDOW
#define IDS_NEW_INCOGNITO_WINDOW IDS_BRAVE_NEW_INCOGNITO_WINDOW

// Keep "Save and share" menu title instead of "Cast, save, and share".
#define IDS_CAST_SAVE_AND_SHARE_MENU_OLD IDS_CAST_SAVE_AND_SHARE_MENU
#undef IDS_CAST_SAVE_AND_SHARE_MENU
#define IDS_CAST_SAVE_AND_SHARE_MENU IDS_SAVE_AND_SHARE_MENU

#define RecentTabsSubMenuModel BraveRecentTabsSubMenuModel
#define BookmarkSubMenuModel BraveBookmarkSubMenuModel
#include "src/chrome/browser/ui/toolbar/app_menu_model.cc"
#undef BookmarkSubMenuModel
#undef RecentTabsSubMenuModel

#undef IDS_CAST_SAVE_AND_SHARE_MENU
#define IDS_CAST_SAVE_AND_SHARE_MENU IDS_CAST_SAVE_AND_SHARE_MENU_OLD
#undef IDS_CAST_SAVE_AND_SHARE_MENU_OLD

#undef IDS_NEW_INCOGNITO_WINDOW
#define IDS_NEW_INCOGNITO_WINDOW IDS_NEW_INCOGNITO_WINDOW_OLD
#undef IDS_NEW_INCOGNITO_WINDOW_OLD

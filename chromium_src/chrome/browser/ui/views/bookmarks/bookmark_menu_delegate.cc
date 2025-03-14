/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_menu_delegate.h"

#include "brave/browser/ui/views/bookmarks/brave_bookmark_context_menu.h"

#define BookmarkContextMenu BraveBookmarkContextMenu
#define GetMaxWidthForMenu GetMaxWidthForMenu_UnUsed
#include "src/chrome/browser/ui/views/bookmarks/bookmark_menu_delegate.cc"
#undef GetMaxWidthForMenu
#undef BookmarkContextMenu

namespace {

constexpr int kBraveMaxMenuWidth = 800;

}  // namespace

int BookmarkMenuDelegate::GetMaxWidthForMenu(MenuItemView* menu) {
  // Chromium limits the width to 400 which causes the menu items to be cut off
  // when displayed in German. Upstream doesn't specify a reason for this size
  // only saying that "IE and FF restrict the max width of a menu". However,
  // MenuDelegate sets the limit to 800 and no other submenu seems to override
  // that value.
  return kBraveMaxMenuWidth;
}

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_BUTTON_BASE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_BUTTON_BASE_H_

#define BookmarkMenuButtonBase BookmarkMenuButtonBase_ChromiumImpl

#include "src/chrome/browser/ui/views/bookmarks/bookmark_menu_button_base.h"  // IWYU pragma: export

#undef BookmarkMenuButtonBase

class BookmarkMenuButtonBase : public BookmarkMenuButtonBase_ChromiumImpl {
  METADATA_HEADER(BookmarkMenuButtonBase, BookmarkMenuButtonBase_ChromiumImpl)

 public:
  BookmarkMenuButtonBase(PressedCallback callback,
                         const std::u16string& title = std::u16string());
  ~BookmarkMenuButtonBase() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_MENU_BUTTON_BASE_H_

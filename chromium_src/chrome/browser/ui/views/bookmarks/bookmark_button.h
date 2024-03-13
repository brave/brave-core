/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BUTTON_H_

#define BookmarkButtonBase BookmarkButtonBase_ChromiumImpl
#define BookmarkButton BookmarkButton_ChromiumImpl
#include "src/chrome/browser/ui/views/bookmarks/bookmark_button.h"  // IWYU pragma: export
#undef BookmarkButton
#undef BookmarkButtonBase

class BookmarkButtonBase : public BookmarkButtonBase_ChromiumImpl {
  METADATA_HEADER(BookmarkButtonBase, BookmarkButtonBase_ChromiumImpl)

 public:
  BookmarkButtonBase(PressedCallback callback, const std::u16string& title);
  ~BookmarkButtonBase() override;
};

// Although BookmarkButton is subclass of BookmarkButtonBase in this upstream
// file, our BookmarkButonBase doesn't become a based class of BookmarkButton as
// both are defined in the same file. So we need to define BookmarkButton also.
class BookmarkButton : public BookmarkButton_ChromiumImpl {
  METADATA_HEADER(BookmarkButton, BookmarkButton_ChromiumImpl)

 public:
  BookmarkButton(PressedCallback callback,
                 const GURL& url,
                 const std::u16string& title,
                 const raw_ptr<Browser> browser);
  ~BookmarkButton() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_BUTTON_H_

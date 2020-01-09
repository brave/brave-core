/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;

import org.chromium.components.bookmarks.BookmarkId;

public class BraveBookmarkUtils extends BookmarkUtils {

  /**
   * Adds a bookmark with the given title and url to the provided parent. Provides
   * no visual feedback that a bookmark has been added.
   *
   * @param title The title of the bookmark.
   * @param url The URL of the new bookmark.
   * @param parent The parentId of the bookmark
   */
  public static BookmarkId addBookmarkSilently(
          Context context, BookmarkModel bookmarkModel, String title, String url, BookmarkId parent) {
      if (parent == null || !bookmarkModel.doesBookmarkExist(parent)) {
          parent = bookmarkModel.getDefaultFolder();
      }

      return bookmarkModel.addBookmark(parent, bookmarkModel.getChildCount(parent), title, url);
  }

}

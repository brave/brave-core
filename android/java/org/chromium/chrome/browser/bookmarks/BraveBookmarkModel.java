/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkId;

// see org.brave.bytecode.BraveBookmarkModelClassAdapter
public class BraveBookmarkModel extends BookmarkModel {

  public void deleteBookmarkSilently(BookmarkId bookmark) {
      assert null != bookmark;
      deleteBookmark(bookmark);
  }

  public void extensiveBookmarkChangesBeginning() {
    assert false;
  }

  public void extensiveBookmarkChangesEnded() {
    assert false;
  }

  public static BookmarkItem createBookmarkItem(long id, int type, String title, String url,
         boolean isFolder, long parentId, int parentIdType, boolean isEditable,
         boolean isManaged) {
    assert false;
    return null;
  }

  /**
   * Calls {@link BookmarkBridge#moveBookmark(BookmarkId, BookmarkId, int)} for the given
   * bookmark. The bookmark is appended at the end. Call that method from Brave's sync only
   */
  public void moveBookmark(BookmarkId bookmarkId, BookmarkId newParentId) {
      assert isBookmarkModelLoaded();
      if (!isBookmarkModelLoaded()) {
          return;
      }
      int appendedIndex = getChildCount(newParentId);
      moveBookmark(bookmarkId, newParentId, appendedIndex);
  }
}

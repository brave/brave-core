/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import org.chromium.base.Log;
import org.chromium.chrome.browser.BraveActivity;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkId;

import java.util.ArrayList;
import java.util.List;

public class BraveBookmarkWorker {
  public static void CreateUpdateBookmark(boolean bCreate, BookmarkItem bookmarkItem) {
      BraveActivity mainActivity = BraveRewardsHelper.getBraveActivity();
      if (null != mainActivity && null != mainActivity.mBraveSyncWorker) {
          mainActivity.mBraveSyncWorker.CreateUpdateBookmark(bCreate, bookmarkItem);
      }
  }

  public static void moveBookmarks(List<BookmarkId> bookmarkIds, BookmarkId newParentId,
        BookmarkModel bookmarkModel) {
      BookmarkItem[] bookmarksToMove = new BookmarkItem[bookmarkIds.size()];
      for (int i = 0; i < bookmarkIds.size(); ++i) {
          bookmarksToMove[i] = bookmarkModel.getBookmarkById(bookmarkIds.get(i));
      }
      BraveActivity mainActivity = BraveRewardsHelper.getBraveActivity();
      if (null != mainActivity && null != mainActivity.mBraveSyncWorker) {
          mainActivity.mBraveSyncWorker.CreateUpdateDeleteBookmarks(
            BraveSyncWorker.UPDATE_RECORD, bookmarksToMove, true, false);
      }
  }

  public static void syncDeletedBookmarks(List<BookmarkItem> bookmarks) {
      if (bookmarks == null || bookmarks.size() == 0) {
          return;
      }
      BraveActivity mainActivity = BraveRewardsHelper.getBraveActivity();
      if (null != mainActivity && null != mainActivity.mBraveSyncWorker) {
          mainActivity.mBraveSyncWorker.DeleteBookmarks(
            bookmarks.toArray(new BookmarkItem[bookmarks.size()]));
      }
  }

  public static List<BookmarkItem> GetChildren(BookmarkItem parent, BookmarkModel bookmarkModel) {
      List<BookmarkItem> res = new ArrayList<BookmarkItem>();
      if (!parent.isFolder()) {
          return res;
      }
      res = bookmarkModel.getBookmarksForFolder(parent.getId());
      List<BookmarkItem> newList = new ArrayList<BookmarkItem>();
      for (BookmarkItem item : res) {
          if (!item.isFolder()) {
              continue;
          }
          newList.addAll(bookmarkModel.getBookmarksForFolder(item.getId()));
      }
      res.addAll(newList);

      return res;
  }
}

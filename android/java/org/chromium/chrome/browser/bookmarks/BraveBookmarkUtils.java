/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.Nullable;

import org.chromium.base.BuildInfo;
import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.appmenu.AppMenuPropertiesDelegateImpl;
import org.chromium.chrome.browser.bookmarks.BookmarkUtils;
import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

/**
 * A class holding static util functions for bookmark.
 */
public class BraveBookmarkUtils extends BookmarkUtils {
    public static void addOrEditBookmark(@Nullable BookmarkItem existingBookmarkItem,
            BookmarkModel bookmarkModel, Tab tab, SnackbarManager snackbarManager,
            BottomSheetController bottomSheetController, Activity activity, boolean fromCustomTab,
            @BookmarkType int bookmarkType, Callback<BookmarkId> callback,
            boolean fromExplicitTrackUi) {
        assert bookmarkModel.isBookmarkModelLoaded();
        if (existingBookmarkItem != null) {
            if (snackbarManager.isShowing()) {
                snackbarManager.dismissAllSnackbars();
            }
            bookmarkModel.deleteBookmark(existingBookmarkItem.getId());
            callback.onResult(existingBookmarkItem.getId());
            return;
        }

        BookmarkUtils.addOrEditBookmark(existingBookmarkItem, bookmarkModel, tab, snackbarManager,
                bottomSheetController, activity, fromCustomTab, bookmarkType, callback,
                fromExplicitTrackUi);
    }
}

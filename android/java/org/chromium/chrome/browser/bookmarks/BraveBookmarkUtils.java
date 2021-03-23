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
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.bookmarks.BookmarkUtils;
import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

/**
 * A class holding static util functions for bookmark.
 */
public class BraveBookmarkUtils extends BookmarkUtils {
    private static final String TAG = "BraveBookmarkUtils";
    /**
     * If the tab has already been bookmarked, start {@link BookmarkEditActivity} for the
     * normal bookmark or show the reading list page for reading list bookmark.
     * If not, add the bookmark to {@link BookmarkModel}, and show a snackbar notifying the user.
     *
     * @param existingBookmarkItem The {@link BookmarkItem} if the tab has already been bookmarked.
     * @param bookmarkModel The bookmark model.
     * @param tab The tab to add or edit a bookmark.
     * @param snackbarManager The {@link SnackbarManager} used to show the snackbar.
     * @param bottomSheetController The {@link BottomSheetController} used to show the bottom sheet.
     * @param activity Current activity.
     * @param fromCustomTab boolean indicates whether it is called by Custom Tab.
     * @param callback Invoked with the resulting bookmark ID, which could be null if unsuccessful.
     */
    public static void addOrEditBookmark(@Nullable BookmarkItem existingBookmarkItem,
            BookmarkModel bookmarkModel, Tab tab, SnackbarManager snackbarManager,
            BottomSheetController bottomSheetController, Activity activity, boolean fromCustomTab,
            Callback<BookmarkId> callback) {
        assert bookmarkModel.isBookmarkModelLoaded();
        if (existingBookmarkItem != null) {
            if (snackbarManager.isShowing()) {
                snackbarManager.dismissAllSnackbars();
            }
            bookmarkModel.deleteBookmark(existingBookmarkItem.getId());
            bookmarkModel.destroy();
            callback.onResult(existingBookmarkItem.getId());
            return;
        }

        if (CachedFeatureFlags.isEnabled(ChromeFeatureList.READ_LATER)) {
            // Show a bottom sheet to let the user select target bookmark folder.
            showBookmarkBottomSheet(bookmarkModel, tab, snackbarManager, bottomSheetController,
                    activity, fromCustomTab, callback);
            return;
        }

        BookmarkId newBookmarkId = addBookmarkAndShowSnackbar(
                bookmarkModel, tab, snackbarManager, activity, fromCustomTab);
        callback.onResult(newBookmarkId);
    }

    protected static void showBookmarkBottomSheet(BookmarkModel bookmarkModel, Tab tab,
            SnackbarManager snackbarManager, BottomSheetController bottomSheetController,
            Activity activity, boolean fromCustomTab, Callback<BookmarkId> callback) {
        assert (false);
    }

    // The legacy code path to add or edit bookmark without triggering the bookmark bottom sheet.
    protected static BookmarkId addBookmarkAndShowSnackbar(BookmarkModel bookmarkModel, Tab tab,
            SnackbarManager snackbarManager, Activity activity, boolean fromCustomTab) {
        assert (false);
        return null;
    }
}

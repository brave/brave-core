/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.app.bookmarks.BraveBookmarkActivity;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

/**
 * A class holding static util functions for bookmark.
 */
public class BraveBookmarkUtils extends BookmarkUtils {
    private static final String TAG = "BraveBookmarkUtils";

    public static void addOrEditBookmark(
            @Nullable BookmarkItem existingBookmarkItem,
            BookmarkModel bookmarkModel,
            Tab tab,
            BottomSheetController bottomSheetController,
            Activity activity,
            @BookmarkType int bookmarkType,
            Callback<BookmarkId> callback,
            boolean fromExplicitTrackUi) {
        assert bookmarkModel.isBookmarkModelLoaded();
        if (existingBookmarkItem != null) {
            bookmarkModel.deleteBookmark(existingBookmarkItem.getId());
            callback.onResult(existingBookmarkItem.getId());
            return;
        }

        BookmarkUtils.addOrEditBookmark(
                existingBookmarkItem,
                bookmarkModel,
                tab,
                bottomSheetController,
                activity,
                bookmarkType,
                callback,
                fromExplicitTrackUi);
    }

    public static void showBookmarkManagerOnPhone(
            Activity activity, String url, boolean isIncognito) {
        Intent intent =
                new Intent(activity == null ? ContextUtils.getApplicationContext() : activity,
                        BraveBookmarkActivity.class);
        intent.putExtra(IntentHandler.EXTRA_INCOGNITO_MODE, isIncognito);
        intent.setData(Uri.parse(url));
        if (activity != null) {
            // Start from an existing activity.
            intent.putExtra(IntentHandler.EXTRA_PARENT_COMPONENT, activity.getComponentName());
            activity.startActivity(intent);
        } else {
            // Start a new task.
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            IntentHandler.startActivityForTrustedIntent(intent);
        }
    }

    public static void showBookmarkImportExportDialog(AppCompatActivity appCompatActivity,
            boolean isImport, boolean isSuccess, String exportFilePath) {
        try {
            BraveBookmarkImportExportDialogFragment dialogFragment =
                    BraveBookmarkImportExportDialogFragment.newInstance(
                            isImport, isSuccess, exportFilePath);
            dialogFragment.show(appCompatActivity.getSupportFragmentManager(),
                    "BraveBookmarkImportExportDialogFragment");
        } catch (IllegalStateException e) {
            Log.e(TAG, "showBookmarkImportExportDialog:" + e.getMessage());
        }
    }

    public static boolean isSpecialFolder(BookmarkModel bookmarkModel, BookmarkItem item) {
        // This is to avoid the root folder to have different color and tint.
        return false;
    }
}

// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;
import android.content.Context;

import org.chromium.base.BuildInfo;
import org.chromium.base.Log;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.bookmarks.BookmarkUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;

/**
 * A class holding static util functions for bookmark.
 */
public class BraveBookmarkUtils {
    private static final String TAG = "BraveBookmarkUtils";
    /**
     * If the tab has already been bookmarked, start {@link BookmarkEditActivity} for the
     * bookmark. If not, add the bookmark to bookmarkmodel, and show a snackbar notifying the user.
     *
     * Note: Takes ownership of bookmarkModel, and will call |destroy| on it when finished.
     *
     * @param existingBookmarkId The bookmark ID if the tab has already been bookmarked.
     * @param bookmarkModel The bookmark model.
     * @param tab The tab to add or edit a bookmark.
     * @param snackbarManager The SnackbarManager used to show the snackbar.
     * @param activity Current activity.
     * @param fromCustomTab boolean indicates whether it is called by Custom Tab.
     * @return Bookmark ID of the bookmark. Could be <code>null</code> if bookmark didn't exist
     *   and bookmark model failed to create it.
     */
    public static BookmarkId addOrEditBookmark(long existingBookmarkId, BookmarkModel bookmarkModel,
            Tab tab, SnackbarManager snackbarManager, Activity activity, boolean fromCustomTab) {
        if (existingBookmarkId != BookmarkId.INVALID_ID) {
            BookmarkId bookmarkId = new BookmarkId(existingBookmarkId, BookmarkType.NORMAL);
            if (snackbarManager.isShowing()) {
                snackbarManager.dismissAllSnackbars();
            }
            bookmarkModel.deleteBookmark(bookmarkId);
            bookmarkModel.destroy();
            return bookmarkId;
        }

        BookmarkId bookmarkId =
                addBookmarkInternal(activity, bookmarkModel, tab.getTitle(), tab.getOriginalUrl());

        Snackbar snackbar = null;
        if (bookmarkId == null) {
            snackbar = Snackbar.make(activity.getString(R.string.bookmark_page_failed),
                                       new SnackbarController() {
                                           @Override
                                           public void onDismissNoAction(Object actionData) {}

                                           @Override
                                           public void onAction(Object actionData) {}
                                       },
                                       Snackbar.TYPE_NOTIFICATION, Snackbar.UMA_BOOKMARK_ADDED)
                               .setSingleLine(false);
            RecordUserAction.record("EnhancedBookmarks.AddingFailed");
        } else {
            String folderName = bookmarkModel.getBookmarkTitle(
                    bookmarkModel.getBookmarkById(bookmarkId).getParentId());
            SnackbarController snackbarController =
                    createSnackbarControllerForEditButton(activity, bookmarkId);
            if (BookmarkUtils.getLastUsedParent(activity) == null) {
                if (fromCustomTab) {
                    String packageLabel = BuildInfo.getInstance().hostPackageLabel;
                    snackbar = Snackbar.make(
                            activity.getString(R.string.bookmark_page_saved, packageLabel),
                            snackbarController, Snackbar.TYPE_ACTION, Snackbar.UMA_BOOKMARK_ADDED);
                } else {
                    snackbar = Snackbar.make(
                            activity.getString(R.string.bookmark_page_saved_default),
                            snackbarController, Snackbar.TYPE_ACTION, Snackbar.UMA_BOOKMARK_ADDED);
                }
            } else {
                snackbar = Snackbar.make(folderName, snackbarController, Snackbar.TYPE_ACTION,
                                           Snackbar.UMA_BOOKMARK_ADDED)
                                   .setTemplateText(
                                           activity.getString(R.string.bookmark_page_saved_folder));
            }
            snackbar.setSingleLine(false).setAction(
                    activity.getString(R.string.bookmark_item_edit), null);
        }
        snackbarManager.showSnackbar(snackbar);

        bookmarkModel.destroy();
        return bookmarkId;
    }

    /**
     * An internal version of {@link #addBookmarkSilently(Context, BookmarkModel, String, String)}.
     * Will reset last used parent if it fails to add a bookmark
     */
    private static BookmarkId addBookmarkInternal(
            Context context, BookmarkModel bookmarkModel, String title, String url) {
        BookmarkId parent = BookmarkUtils.getLastUsedParent(context);
        BookmarkItem parentItem = null;
        if (parent != null) {
            parentItem = bookmarkModel.getBookmarkById(parent);
        }
        if (parent == null || parentItem == null || parentItem.isManaged()
                || !parentItem.isFolder()) {
            parent = bookmarkModel.getDefaultFolder();
        }
        BookmarkId bookmarkId =
                bookmarkModel.addBookmark(parent, bookmarkModel.getChildCount(parent), title, url);

        // TODO(lazzzis): remove log after bookmark sync is fixed, crbug.com/986978
        if (bookmarkId == null) {
            Log.e(TAG,
                    "Failed to add bookmarks: parentTypeAndId %s, defaultFolderTypeAndId %s, "
                            + "mobileFolderTypeAndId %s, parentEditable Managed isFolder %s,",
                    parent, bookmarkModel.getDefaultFolder(), bookmarkModel.getMobileFolderId(),
                    parentItem == null ? "null"
                                       : (parentItem.isEditable() + " " + parentItem.isManaged()
                                               + " " + parentItem.isFolder()));
            BookmarkUtils.setLastUsedParent(context, bookmarkModel.getDefaultFolder());
        }
        return bookmarkId;
    }

    /**
     * Creates a snackbar controller for a case where "Edit" button is shown to edit the newly
     * created bookmark.
     */
    private static SnackbarController createSnackbarControllerForEditButton(
            final Activity activity, final BookmarkId bookmarkId) {
        return new SnackbarController() {
            @Override
            public void onDismissNoAction(Object actionData) {
                RecordUserAction.record("EnhancedBookmarks.EditAfterCreateButtonNotClicked");
            }

            @Override
            public void onAction(Object actionData) {
                RecordUserAction.record("EnhancedBookmarks.EditAfterCreateButtonClicked");
                BookmarkUtils.startEditActivity(activity, bookmarkId);
            }
        };
    }
}

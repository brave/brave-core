/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.content.Context;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.browser_ui.widget.dragreorder.DragReorderableRecyclerViewAdapter;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableListLayout;
import org.chromium.components.browser_ui.widget.selectable_list.SelectableListToolbar.SearchDelegate;
import org.chromium.components.browser_ui.widget.selectable_list.SelectionDelegate;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;

class BraveBookmarkToolbarCoordinator extends BookmarkToolbarCoordinator {
    // Overridden Chromium's BookmarkToolbarCoordinator.mToolbar
    private BookmarkToolbar mToolbar;

    BraveBookmarkToolbarCoordinator(
            Context context,
            SelectableListLayout<BookmarkId> selectableListLayout,
            SelectionDelegate<BookmarkId> selectionDelegate,
            SearchDelegate searchDelegate,
            DragReorderableRecyclerViewAdapter dragReorderableRecyclerViewAdapter,
            boolean isDialogUi,
            OneshotSupplier<BookmarkDelegate> bookmarkDelegateSupplier,
            BookmarkModel bookmarkModel,
            BookmarkOpener bookmarkOpener,
            BookmarkUiPrefs bookmarkUiPrefs,
            ModalDialogManager modalDialogManager,
            Runnable endSearchRunnable,
            BookmarkMoveSnackbarManager bookmarkMoveSnackbarManager,
            BooleanSupplier incognitoEnabledSupplier) {
        super(
                context,
                selectableListLayout,
                selectionDelegate,
                searchDelegate,
                dragReorderableRecyclerViewAdapter,
                isDialogUi,
                bookmarkDelegateSupplier,
                bookmarkModel,
                bookmarkOpener,
                bookmarkUiPrefs,
                modalDialogManager,
                endSearchRunnable,
                bookmarkMoveSnackbarManager,
                incognitoEnabledSupplier);

        if (mToolbar instanceof BraveBookmarkToolbar) {
            ((BraveBookmarkToolbar) mToolbar).setBraveBookmarkDelegate(bookmarkDelegateSupplier);
        }
    }
}

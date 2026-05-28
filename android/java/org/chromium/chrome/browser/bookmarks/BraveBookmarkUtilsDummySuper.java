/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import android.app.Activity;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.price_tracking.PriceDropNotificationManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkItem;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

import java.util.List;

/**
 * Holds same-named stubs that bytecode-redirect to private members of {@link BookmarkUtils}, so
 * {@link BraveBookmarkUtils} can back-call them despite Java visibility rules. The stub bodies are
 * never executed: {@code BraveBookmarkUtilsClassAdapter} rewrites each call site to invoke the
 * upstream method (which it also bumps to public at bytecode time).
 */
@NullMarked
class BraveBookmarkUtilsDummySuper extends BookmarkUtils {
    private BraveBookmarkUtilsDummySuper() {}

    public static void addOrEditSingleBookmark(
            @Nullable BookmarkItem existingBookmarkItem,
            BookmarkModel bookmarkModel,
            Tab tab,
            BottomSheetController bottomSheetController,
            Activity activity,
            @BookmarkType int bookmarkType,
            Callback<List<@Nullable BookmarkId>> callback,
            boolean fromExplicitTrackUi,
            BookmarkManagerOpener bookmarkManagerOpener,
            PriceDropNotificationManager priceDropNotificationManager,
            boolean isBookmarkBarVisible) {
        assert false : "This class usage should be removed via bytecode modification!";
    }
}
